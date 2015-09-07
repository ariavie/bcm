/*
 * $Id: hashing.c,v 1.2 Broadcom SDK $
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
 * Purpose:     TD2-Hash calcualtions for trunk and ECMP packets.
 */

#include <shared/bsl.h>

#include <soc/drv.h>
#include <soc/hash.h>
#include <bcm/switch.h>
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
    uint32 rtag7_port_lbn;              /* port LBN value from the port table*/
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

#define RTAG7_RH_MODE_ECMP    0
#define RTAG7_RH_MODE_LAG     1
#define RTAG7_RH_MODE_HGT     2

#define RTAG7_L2_ONLY         0x0
#define RTAG7_UNKNOWN_HIGIG   0x1
#define RTAG7_MPLS            0x2
#define RTAG7_MIM             0x3
#define RTAG7_IPV4            0x4
#define RTAG7_IPV6            0x5
#define RTAG7_FCOE            0x6
#define RTAG7_TRILL           0x7

#define ENHANCED_HASHING_CONTROLRH_FLOWSET_TABLE_CONFIG_ENCODING_HALF_AND_HALF 0
#define ENHANCED_HASHING_CONTROLRH_FLOWSET_TABLE_CONFIG_ENCODING_ALL_ECMP      1
#define ENHANCED_HASHING_CONTROLRH_FLOWSET_TABLE_CONFIG_ENCODING_ALL_LAG       2

#define RH_LAG_FLOWSET_MOD_ID_SHIFT_VAL 7

#define HASH_VALUE_64_COMPUTE(subfield,offset,width) do { \
        uint64 sf; \
        COMPILER_64_ZERO(sf); \
        COMPILER_64_ADD_64(sf, subfield); \
        COMPILER_64_SHR(subfield, offset); \
        COMPILER_64_SHL(sf, (width - offset)); \
        COMPILER_64_OR(subfield, sf); \
    } while(0)

#define HASH_VALUE_32_COMPUTE(hash_value_32, hash_value_64) do { \
        COMPILER_64_TO_32_LO(hash_value_32, hash_value_64); \
    } while(0)


/*
 * Function:
 *          main_td2_do_rtag7_hashing
 * Description:
 *          read hash control register values and
 *          set hash_res' fields
 * Parameters:
 *          unit - StrataSwitch PCI device unit number (driver internal).
 *          pkt_info - ptr to data that has packet information
 *          hash_res - internal data where hashing informaiton is stored 
 * Returns:
 *      BCM_E_xxxx
 */
STATIC int
main_td2_do_rtag7_hashing(int unit,
                       bcm_switch_pkt_info_t *pkt_info,
                       bcm_rtag7_base_hash_t *hash_res)
{
    /* regular var declaration */
    uint32      hash_src_port;
    uint32      hash_src_modid;
    uint32      rtag7_a_sel;
    bcm_ip_t            sip_a;       /* Source IPv4 address. */
    bcm_ip_t            dip_a;       /* Destination IPv4 address. */
    uint32      rtag7_b_sel;
    bcm_ip_t            sip_b;       /* Source IPv4 address. */
    bcm_ip_t            dip_b;       /* Destination IPv4 address. */
    uint32      hash_field_bmap_a;   /* block A chosen bitmap*/
    uint32      hash_field_bmap_b;   /* block B chosen bitmap*/
    
    uint32 hash_word[7];
    uint32 hash[13];
    int    elem, elem_bit;
    uint32 hash_element_valid_bits;
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
    int symmetric_hash_control_mask = 0;
    int symmetric_hash_need_swap_ip6 = 0;
    int symmetric_hash_need_swap_ip4 = 0;

    /* read RTAG7 hash control register values */
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

    /* read RTAG7 fields from hash control registers */
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
    
    /* read RTAG7 fields from hash control3 registers */
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
    
    /* read RTAG7 fields from hash control2 registers */
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROL_2r,
                            ENABLE_FLOW_LABEL_IPV6_Af)) {
        rtag7_en_flow_label_ipv6_a =
            soc_reg_field_get(unit, RTAG7_HASH_CONTROL_2r,
                              rtag7_hash_control_2,
                              ENABLE_FLOW_LABEL_IPV6_Af);
    } else {
        rtag7_en_flow_label_ipv6_a = 0; 
    }
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROL_2r,
                            ENABLE_FLOW_LABEL_IPV6_Bf)) {
        rtag7_en_flow_label_ipv6_b =
            soc_reg_field_get(unit, RTAG7_HASH_CONTROL_2r,
                              rtag7_hash_control_2,
                              ENABLE_FLOW_LABEL_IPV6_Bf);
    } else {
        rtag7_en_flow_label_ipv6_b = 0; 
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
        rtag7_macro_flow_hash_function_select = 0; 
    }
    if (SOC_REG_FIELD_VALID(unit, RTAG7_HASH_CONTROL_2r,
                            MACRO_FLOW_HASH_BYTE_SELf)) {
        rtag7_macro_flow_hash_byte_sel =
            soc_reg_field_get(unit, RTAG7_HASH_CONTROL_2r,
                              rtag7_hash_control_2,
                              MACRO_FLOW_HASH_BYTE_SELf);
    } else {
        rtag7_macro_flow_hash_byte_sel = 0; 
    }

    /* read RTAG7 fields from hash seed registers */
    rtag7_hash_seed_a =
        soc_reg_field_get(unit, RTAG7_HASH_SEED_Ar, rtag7_hash_seed_a_reg,
                          HASH_SEED_Af);
    rtag7_hash_seed_b =
        soc_reg_field_get(unit, RTAG7_HASH_SEED_Br, rtag7_hash_seed_b_reg,
                          HASH_SEED_Bf);

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (BCM_SUCCESS(bcm_esw_switch_control_get(unit,
            bcmSwitchSymmetricHashControl, &symmetric_hash_control_mask))) {
        if (symmetric_hash_control_mask & 
            (BCM_SYMMETRIC_HASH_0_IP6_ENABLE | BCM_SYMMETRIC_HASH_1_IP6_ENABLE)) {
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_IPV6) && 
                _BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_IPV6) &&
                (sal_memcmp(pkt_info->sip6, pkt_info->dip6, BCM_IP6_ADDRLEN) < 0)) {
                symmetric_hash_need_swap_ip6 = 1;
            }
        }
        if (symmetric_hash_control_mask & 
            (BCM_SYMMETRIC_HASH_0_IP4_ENABLE | BCM_SYMMETRIC_HASH_1_IP4_ENABLE)) {
            if (_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_IP) && 
                _BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, DST_IP) &&
                (pkt_info->sip < pkt_info->dip)) {
                symmetric_hash_need_swap_ip4 = 1;
            }
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    /* RTAG7 tables */
    /* the first step is to choose the paket type and to fold the ipv6
     * addresses in case of IPv6 packet*/   

    /* option A ip folding for rtag 7 */
    if ((_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, ETHERTYPE) &&
         (pkt_info->ethertype == ETHERTYPE_IPV6)) &&
               (rtag7_disable_hash_ipv6_a == 0)) {

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

        /* Symmetric hash swap src/dst ipaddr/l4_port for option A */
        if (((rtag7_a_sel == RTAG7_IPV6) && 
            symmetric_hash_need_swap_ip6 && 
            (symmetric_hash_control_mask & BCM_SYMMETRIC_HASH_0_IP6_ENABLE)) 
            ||
            ((rtag7_a_sel == RTAG7_IPV4) && 
            symmetric_hash_need_swap_ip4 && 
            (symmetric_hash_control_mask & BCM_SYMMETRIC_HASH_0_IP4_ENABLE))) {

            uint32 temp;
            temp = hash[11]; hash[11] = hash[9]; hash[9] = temp;
            temp = hash[10]; hash[10] = hash[8]; hash[8] = temp;
            temp = hash[6]; hash[6] = hash[5]; hash[5] = temp;
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "Unit %d - Symmetric hash swap for hash A calculation.\n"),
                         unit));
        }

        if ((rtag7_a_sel == RTAG7_IPV6) && rtag7_en_flow_label_ipv6_a) {
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "Unit %d - Hash calculation: The system is set to use ipv6 flow label" 
                                     " and the code can't get this info\n"), unit));
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
                    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                                (BSL_META_U(unit,
                                            "Unit %d - Hash calculation: Bitmap is block A IPv4 TCP=UDP\n"),
                                 unit));
                } else {
                    sc_reg = RTAG7_IPV4_TCP_UDP_HASH_FIELD_BMAP_2r;
                    sc_field = IPV4_TCP_UDP_FIELD_BITMAP_Af;
                    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                                (BSL_META_U(unit,
                                            "Unit %d - Hash calculation: Bitmap is block A IPv4 L4 tcp/udp\n"),
                                 unit));
                }
            } else {
                sc_reg = RTAG7_HASH_FIELD_BMAP_1r;
                sc_field = IPV4_FIELD_BITMAP_Af;
                LOG_VERBOSE(BSL_LS_BCM_COMMON,
                            (BSL_META_U(unit,
                                        "Unit %d - Hash calculation: Bitmap is block A IPv4 \n"),
                             unit));
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
                    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                                (BSL_META_U(unit,
                                            "Unit %d - Hash calculation: Bitmap is block A IPv6 TCP=UDP\n"),
                                 unit));
                } else {
                    sc_reg = RTAG7_IPV6_TCP_UDP_HASH_FIELD_BMAP_2r;
                    sc_field = IPV6_TCP_UDP_FIELD_BITMAP_Af;
                    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                                (BSL_META_U(unit,
                                            "Unit %d - Hash calculation: Bitmap is block A IPv6 L4 tcp/udp\n"),
                                 unit));
                }
            } else {
                sc_reg = RTAG7_HASH_FIELD_BMAP_2r;
                sc_field = IPV6_FIELD_BITMAP_Af;
                LOG_VERBOSE(BSL_LS_BCM_COMMON,
                            (BSL_META_U(unit,
                                        "Unit %d - Hash calculation: Bitmap is block A IPv6 \n"),
                             unit));
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
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Unit %d - Hash calculation: Bitmap is block A L2\n"),
                     unit));
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

    hash_crc16_bisync = _shr_crc16_draco_array(hash_word, 28);
    hash_crc16_ccitt =  _shr_crc16_ccitt_array(hash_word, 28);
    hash_crc32 =        _shr_crc32_castagnoli_array(hash_word, 28);

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

        /* Symmetric hash swap src/dst ipaddr/l4_port for option B */
        if (((rtag7_b_sel == RTAG7_IPV6) && 
            symmetric_hash_need_swap_ip6 && 
            (symmetric_hash_control_mask & BCM_SYMMETRIC_HASH_1_IP6_ENABLE)) 
            ||
            ((rtag7_b_sel == RTAG7_IPV4) && 
            symmetric_hash_need_swap_ip4 && 
            (symmetric_hash_control_mask & BCM_SYMMETRIC_HASH_1_IP4_ENABLE))) {

            uint32 temp;
            temp = hash[11]; hash[11] = hash[9]; hash[9] = temp;
            temp = hash[10]; hash[10] = hash[8]; hash[8] = temp;
            temp = hash[6]; hash[6] = hash[5]; hash[5] = temp;
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "Unit %d - Symmetric hash swap for hash B calculation.\n"),
                         unit));
        }

        if ((rtag7_b_sel == RTAG7_IPV6) && rtag7_en_flow_label_ipv6_b) {
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "Unit %d - Hash calculation: The system is set to use ipv6 flow label" 
                                     " and the code can't get this info\n"), unit));
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
                    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                                (BSL_META_U(unit,
                                            "Unit %d - Hash calculation: Bitmap is block B IPv4 TCP=UDP\n"),
                                 unit));
                } else {
                    sc_reg = RTAG7_IPV4_TCP_UDP_HASH_FIELD_BMAP_2r;
                    sc_field = IPV4_TCP_UDP_FIELD_BITMAP_Bf;
                    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                                (BSL_META_U(unit,
                                            "Unit %d - Hash calculation: Bitmap is block B IPv4 L4 tcp/udp\n"),
                                 unit));
                }
            } else {
                sc_reg = RTAG7_HASH_FIELD_BMAP_1r;
                sc_field = IPV4_FIELD_BITMAP_Bf;
                LOG_VERBOSE(BSL_LS_BCM_COMMON,
                            (BSL_META_U(unit,
                                        "Unit %d - Hash calculation: Bitmap is block B IPv4\n"),
                             unit));
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
                    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                                (BSL_META_U(unit,
                                            "Unit %d - Hash calculation: Bitmap is block B IPv6 TCP=UDP\n"),
                                 unit));
                } else {
                    sc_reg = RTAG7_IPV6_TCP_UDP_HASH_FIELD_BMAP_2r;
                    sc_field = IPV6_TCP_UDP_FIELD_BITMAP_Bf;
                    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                                (BSL_META_U(unit,
                                            "Unit %d - Hash calculation: Bitmap is block B IPv6 TCP=UDP\n"),
                                 unit));
                }
            } else {
                sc_reg = RTAG7_HASH_FIELD_BMAP_2r;
                sc_field = IPV6_FIELD_BITMAP_Bf;
                LOG_VERBOSE(BSL_LS_BCM_COMMON,
                            (BSL_META_U(unit,
                                        "Unit %d - Hash calculation: Bitmap is block B IPv6\n"),
                             unit));
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
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Unit %d - Hash calculation: Bitmap is block B L2\n"),
                     unit));
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

    hash_crc16_bisync = _shr_crc16_draco_array(hash_word, 28);
    hash_crc16_ccitt =  _shr_crc16_ccitt_array(hash_word, 28);
    hash_crc32 =        _shr_crc32_castagnoli_array(hash_word, 28);

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

/*
 * Function:
 *          select_td2_hash_subfield
 * Description:
 *          read hashing values of hash A0,A1,B0,B1,LBID
 *          from sub_sel_block pointed by hash_sub_sel
 * Parameters:
 *          concat - tell whether is in concatenated mode or not
 *          hash_sub_sel - tell which hash sub to choose
 *          seleted_subfiled - saves subfield value
 *          hash_Base - internal data where hashing informaiton is stored
 * Returns:
 *      BCM_E_xxxx
 * Note: if hash_sub_sel and hash_Base->hash_x_valid mismatches, 
 *       it will return BCM_E_PARAM
 */
STATIC int
select_td2_hash_subfield(uint32 concat, int hash_sub_sel, uint64 *selected_subfield,
                     bcm_rtag7_base_hash_t *hash_Base)
{
    int rv = BCM_E_NONE;

    switch (hash_sub_sel) {
        case 0: /* select pkt hash A */
            if (concat) {
                COMPILER_64_SET(*selected_subfield,
                    (hash_Base->rtag7_hash16_value_b_1 << 16 | hash_Base->rtag7_hash16_value_b_0),
                    (hash_Base->rtag7_hash16_value_a_1 << 16 | hash_Base->rtag7_hash16_value_a_0));
                if (!hash_Base->hash_a_valid || !hash_Base->hash_b_valid) {
                    /* Missing paramter in input */
                    rv = BCM_E_PARAM;
                }
            } else {
                COMPILER_64_SET(*selected_subfield, 0, hash_Base->rtag7_hash16_value_a_0);
                if (!hash_Base->hash_a_valid) {
                    /* Missing paramter in input */
                    rv = BCM_E_PARAM;
                }
            }
            break;

        case 1: /* select pkt hash B */
            if (concat) {
                COMPILER_64_SET(*selected_subfield, 0, hash_Base->rtag7_port_lbn);
            } else {
                COMPILER_64_SET(*selected_subfield, 0, hash_Base->rtag7_hash16_value_b_0);
                if (!hash_Base->hash_b_valid) {
                    /* Missing paramter in input */
                    rv = BCM_E_PARAM;
                }
            }
            break;

        case 2: /* select port LBN */
            COMPILER_64_SET(*selected_subfield, 0, hash_Base->rtag7_port_lbn);
            break;

        case 3: /* select destination port id or hash A */
            if (concat) {
                COMPILER_64_SET(*selected_subfield, 0, hash_Base->rtag7_port_lbn);
            } else {
                COMPILER_64_SET(*selected_subfield, 0, hash_Base->rtag7_hash16_value_a_0);
                if (!hash_Base->hash_a_valid) {
                    /* Missing paramter in input */
                    rv = BCM_E_PARAM;
                }
            }
            break;

        case 4: /* select LBID from module header or from SW1 stage */ 
            COMPILER_64_SET(*selected_subfield, 0, hash_Base->rtag7_lbid_hash);
            if (!hash_Base->lbid_hash_valid) {
                /* LBID setting isn't RTAG7 */
                rv = BCM_E_CONFIG;
            }
            break;

        case 5: /* select LBID from SW1 stage */
            COMPILER_64_SET(*selected_subfield, 0, hash_Base->rtag7_lbid_hash);
            if (!hash_Base->lbid_hash_valid) {
                /* LBID setting isn't RTAG7 */
                rv = BCM_E_CONFIG;
            }
            break;

        case 6:
            if (concat) {
                COMPILER_64_ZERO(*selected_subfield);
            } else {
                COMPILER_64_SET(*selected_subfield, 0, hash_Base->rtag7_hash16_value_a_1);
                if (!hash_Base->hash_a_valid) {
                    /* Missing paramter in input */
                    rv = BCM_E_PARAM;
                }
            }
            break;

        case 7: 
            if (concat) {
                COMPILER_64_ZERO(*selected_subfield);
            } else {
                COMPILER_64_SET(*selected_subfield, 0, hash_Base->rtag7_hash16_value_b_1);
                if (!hash_Base->hash_b_valid) {
                    /* Missing paramter in input */
                    rv = BCM_E_PARAM;
                }
            }
            break;

        default:
            return BCM_E_PARAM;
    }

    return rv;
}

/*
 * Function:
 *          main_td2_compute_lbid
 * Description:
 *     get lbid value that is used in hashing calculation
 *     if flow hash is used, get SUB_SEL_LBID_OR_ENTROPY_LABELf and 
 *        OFFSET_LBID_OR_ENTROPY_LABELf from RTAG7_FLOW_BASED_HASHm 
 *     if flow hash is not used, get SUB_SEL_LBID_(non)UCf and 
 *        OFFSET_LBID_(non)UCf from RTAG7_PORT_BASED_HASHm 
 * Parameters:
 *          unit - StrataSwitch PCI device unit number (driver internal).
 *          hash_Base - internal data that lbid_hash_value is stored 
 * Returns:
 *      BCM_E_xxxx
 * Note: bcmSwitchLoadBalanceHashSelect must be set to 7 
 *   and bcmSwitchLoadBalanceHashSet0UnicastOffset must be set properly.    
 *       if lbid_hash_sub_sel and hash_Base->hash_x_valid mismatches, 
 *       then lbid_hash_val saved in hash_Base will be 0
 *
 */
STATIC int
main_td2_compute_lbid(int unit, bcm_rtag7_base_hash_t *hash_Base)
{
    /*regular var declaration*/
    uint32 lbid_hash_sub_sel;
    uint32 lbid_hash_offset;
    uint32 lbid_hash_value = 0;

    /*register var declaration*/
    int lbid_rtag = 0;
    uint64 ing_hash_config_reg;
    int    rv = BCM_E_NONE;
    uint32 rtag7_hash_sel;
    uint32 port_based_hash_index;
    rtag7_port_based_hash_entry_t
           port_based_hash_entry;
    rtag7_flow_based_hash_entry_t 
            flow_based_hash_entry;
    uint8   loadbalance_use_flow_hash_uc = 0;
    uint8   loadbalance_use_flow_hash_nonuc = 0;
 
    /* get the lbid rtag value */ 
    if (SOC_REG_FIELD_VALID(unit, ING_CONFIG_64r, LBID_RTAGf)) {
        rv = READ_ING_CONFIG_64r(unit, &ing_hash_config_reg);
        if (BCM_SUCCESS(rv)) {
            lbid_rtag = soc_reg64_field32_get(unit, ING_CONFIG_64r, 
                               ing_hash_config_reg, LBID_RTAGf);  
        } else {
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "Unit %d -compute_lbid fail, lbid_rtag=0\n"),
                         unit));
            lbid_rtag =0;
        }
    } else {
        rv = (BCM_E_UNAVAIL);
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - lbid_rtag = %d\n"),
                 unit, lbid_rtag));
    /* ******************* LBID computation ************************* */

    if (lbid_rtag == 7) {
        /* check if flow hash is used for UC*/
        SOC_IF_ERROR_RETURN
            (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel));
        if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_LBID_UCf)) {
             loadbalance_use_flow_hash_uc =
                soc_reg_field_get(unit, RTAG7_HASH_SELr,
                              rtag7_hash_sel, USE_FLOW_SEL_LBID_UCf);
        } else {
            loadbalance_use_flow_hash_uc = 0; 
        }    
 
        /* check if flow hash is used for NONUC*/
        SOC_IF_ERROR_RETURN
            (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel));
        if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_LBID_NONUCf)) {
            loadbalance_use_flow_hash_nonuc =
                soc_reg_field_get(unit, RTAG7_HASH_SELr,
                              rtag7_hash_sel, USE_FLOW_SEL_LBID_NONUCf);
        } else {
            loadbalance_use_flow_hash_nonuc = 0; 
        }    

        /* RTAG7 hash computation */
        if ((loadbalance_use_flow_hash_nonuc&& (hash_Base->is_nonuc == 0)) || 
            (loadbalance_use_flow_hash_uc && (hash_Base->is_nonuc != 0))) {
            SOC_IF_ERROR_RETURN(
                READ_RTAG7_FLOW_BASED_HASHm(unit, MEM_BLOCK_ANY,
                        (hash_Base->rtag7_macro_flow_id & 0xff),
                         &flow_based_hash_entry));

            /* in RTAG7_FLOW_BASED_HASHm, using same selection for 
               UC and NONUC */
            lbid_hash_sub_sel = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                           &flow_based_hash_entry, SUB_SEL_LBID_OR_ENTROPY_LABELf);
            lbid_hash_offset  = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                           &flow_based_hash_entry, OFFSET_LBID_OR_ENTROPY_LABELf);
         
        } else if (SOC_MEM_IS_VALID(unit, RTAG7_PORT_BASED_HASHm)) {
            if (hash_Base->dev_src_port < 0) {
                int field_count = 0;
                soc_field_t fields[2];
                uint32 values[2];
                bcm_gport_t gport;

                BCM_GPORT_PROXY_SET(gport, 
                    hash_Base->src_modid, hash_Base->src_port);

                if (hash_Base->is_nonuc) {
                    fields[0] = SUB_SEL_LBID_NONUCf ;
                    field_count++;
                    fields[1] = OFFSET_LBID_NONUCf;
                    field_count++;
                } else {
                    fields[0] = SUB_SEL_LBID_UCf ;
                    field_count++;
                    fields[1] = OFFSET_LBID_UCf;
                    field_count++;
                }
                BCM_IF_ERROR_RETURN
                    (bcm_esw_port_lport_fields_get(unit, gport, 
                                                   LPORT_PROFILE_RTAG7_TAB,
                                                   field_count, fields, values));
                lbid_hash_sub_sel = values[0];
                lbid_hash_offset  = values[1];
            } else {
                port_based_hash_index = hash_Base->dev_src_port + 
                                        soc_mem_index_count(unit, LPORT_TABm);
                SOC_IF_ERROR_RETURN(
                    READ_RTAG7_PORT_BASED_HASHm(unit, MEM_BLOCK_ANY,
                                port_based_hash_index, &port_based_hash_entry));
                if (hash_Base->is_nonuc) {
                    lbid_hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                        &port_based_hash_entry, SUB_SEL_LBID_NONUCf);
                    lbid_hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                        &port_based_hash_entry, OFFSET_LBID_NONUCf);
                } else {
                    lbid_hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                        &port_based_hash_entry, SUB_SEL_LBID_UCf);
                    lbid_hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                        &port_based_hash_entry, OFFSET_LBID_UCf);
                }
            }
        } else {
            lbid_hash_sub_sel = 0;
            lbid_hash_offset  = 0;
        }

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
        
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Unit %d - Hash calculation: This function doesn't support rtag 0 6" 
                                 " pls change register ING_CONFIG.LBID_RTAG to value 7\n"), unit));
        hash_Base->rtag7_lbid_hash = 0;
        hash_Base->lbid_hash_valid = FALSE;
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "unit %d - lbid_hash_val=%d, valid=%d\n"),
                 unit, 
                 hash_Base->rtag7_lbid_hash, hash_Base->lbid_hash_valid));

    return rv;
    /* ******************* END of LBID computation ************************* */
}

#ifdef INCLUDE_L3
/*
 * Function:
 *          compute_td2_ecmp_hash
 * Description:
 *     get ecmp hash value using SUB_SEL_ECMPf and OFFSET_ECMPf
 *     if flow hash is used, get subsel and offset from RTAG7_FLOW_BASED_HASHm 
 *     if flow hash is not used, get subsel and offset from RTAG7_PORT_BASED_HASHm 
 * Parameters:
 *          unit - StrataSwitch PCI device unit number (driver internal).
 *          hash_Base - internal data where ecmp hash value is stored 
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
compute_td2_ecmp_hash(int unit, bcm_rtag7_base_hash_t *hash_Base,
                  uint32 *hash_value)
{
    /*regular var declaration*/
    uint32 hash_sub_sel;
    uint32 hash_offset;
    uint64 hash_subfield;
    uint32 hash_subfield_width;
    uint8  rtag7_ecmp_use_macro_flow_hash;
    uint8  ecmp_hash_use_rtag7;
    uint32 rtag7_hash_sel;
    uint32 hash_control;
    uint32 concat;
    uint32 port_based_hash_index;
    rtag7_port_based_hash_entry_t
           port_based_hash_entry;
    rtag7_flow_based_hash_entry_t 
           flow_based_hash_entry;

    /* Select between non-RTAG7 hash value and RTAG7 hash value */
    SOC_IF_ERROR_RETURN
        (READ_HASH_CONTROLr(unit, &hash_control));
    ecmp_hash_use_rtag7 =
        soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                          ECMP_HASH_USE_RTAG7f);

    if (ecmp_hash_use_rtag7 == 0) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Unit %d - ECMP Hash calculation:  non rtag7 calc not supported\n"),
                     unit));
        *hash_value =  0;
        return BCM_E_NONE;
    }

    /* check if flow hash is used*/
    SOC_IF_ERROR_RETURN
        (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel));
    if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_ECMPf)) {
        rtag7_ecmp_use_macro_flow_hash =
            soc_reg_field_get(unit, RTAG7_HASH_SELr,
                              rtag7_hash_sel, USE_FLOW_SEL_ECMPf);
    } else {
        rtag7_ecmp_use_macro_flow_hash = 0;
    }

    /* RTAG7 hash computation */
    if (rtag7_ecmp_use_macro_flow_hash) {
        SOC_IF_ERROR_RETURN(
            READ_RTAG7_FLOW_BASED_HASHm(unit, MEM_BLOCK_ANY,
                    (hash_Base->rtag7_macro_flow_id & 0xff),
                     &flow_based_hash_entry));    

        /* Generating RTAG7 Macro Flow Based Hash Subsel and Offset for use in RSEL1 Stage */
        /* For future designs this logic can be placed in RSEL1 */
        hash_sub_sel = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                           &flow_based_hash_entry, SUB_SEL_ECMPf);
        hash_offset  = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                           &flow_based_hash_entry, OFFSET_ECMPf);
        concat       = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                           &flow_based_hash_entry, CONCATENATE_HASH_FIELDS_ECMPf);
    } else if (SOC_MEM_IS_VALID(unit, RTAG7_PORT_BASED_HASHm)) {     
        if (hash_Base->dev_src_port < 0) {
            int field_count = 0;
            soc_field_t fields[3];
            uint32 values[3];
            bcm_gport_t gport;
        
            BCM_GPORT_PROXY_SET(gport, 
                hash_Base->src_modid, hash_Base->src_port);
        
            fields[0] = SUB_SEL_ECMPf;
            field_count++;
            fields[1] = OFFSET_ECMPf;
            field_count++;
            fields[2] = CONCATENATE_HASH_FIELDS_ECMPf;
            field_count++;
            BCM_IF_ERROR_RETURN
                (bcm_esw_port_lport_fields_get(unit, gport, 
                                               LPORT_PROFILE_RTAG7_TAB,
                                               field_count, fields, values));
            hash_sub_sel = values[0];
            hash_offset  = values[1];
            concat       = values[2];
        } else {
            port_based_hash_index = hash_Base->dev_src_port + 
                                    soc_mem_index_count(unit, LPORT_TABm);      
            SOC_IF_ERROR_RETURN(     
                READ_RTAG7_PORT_BASED_HASHm(unit, MEM_BLOCK_ANY, 
                            port_based_hash_index, &port_based_hash_entry));

            hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                    &port_based_hash_entry, SUB_SEL_ECMPf);
            hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, OFFSET_ECMPf);
            concat       = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, CONCATENATE_HASH_FIELDS_ECMPf);
        }
    } else {
        hash_sub_sel = 0;
        hash_offset  = 0;
        concat       = 0;
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - ecmp hash_seb_sel=%d, hash_offset=%d, concat=%d\n"),
                 unit, hash_sub_sel, hash_offset, concat));
    BCM_IF_ERROR_RETURN
        (select_td2_hash_subfield(concat, hash_sub_sel, &hash_subfield, hash_Base));

    hash_subfield_width = concat ? 64 : 16;

    /* Barrel shift the hash subfield, then take the LSB 16 bits */
    HASH_VALUE_64_COMPUTE(hash_subfield, hash_offset, hash_subfield_width);
    HASH_VALUE_32_COMPUTE(*hash_value, hash_subfield);
    *hash_value &= 0xffff;  

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - ecmp hash val=%d\n"),
                 unit,  *hash_value));

    return BCM_E_NONE;
    /* ***************** END of ECMP HASH CALCULATIONS.  ************************ */
}

/*
 * Function:
 *          compute_td2_ecmp_rh_hash
 * Description:
 *     get ecmp rh hash value using SUB_SEL_RH_ECMPf and OFFSET_RH_ECMPf 
 *     if flow hash is used, get subsel and offset from RTAG7_FLOW_BASED_HASHm 
 *     if flow hash is not used, get subsel and offset from RTAG7_PORT_BASED_HASHm 
 * Parameters:
 *          unit - StrataSwitch PCI device unit number (driver internal).
 *          hash_Base - internal data where rh ecmp hash value is stored 
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
compute_td2_ecmp_rh_hash(int unit, bcm_rtag7_base_hash_t *hash_Base,
                  uint32 *hash_value)
{
    /*regular var declaration*/
    uint32 hash_sub_sel;
    uint32 hash_offset;
    uint64 hash_subfield;
    uint32 hash_subfield_width;
    uint8  rtag7_ecmp_use_macro_flow_hash;
    uint8  ecmp_hash_use_rtag7;
    uint32 rtag7_hash_sel;
    uint32 hash_control;
    uint32 concat;
    rtag7_flow_based_hash_entry_t 
           flow_based_hash_entry;
    rtag7_port_based_hash_entry_t
            port_based_hash_entry;
    uint32 port_based_hash_index;

    /* Select between non-RTAG7 hash value and RTAG7 hash value */
    SOC_IF_ERROR_RETURN
        (READ_HASH_CONTROLr(unit, &hash_control));
    ecmp_hash_use_rtag7 =
        soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                          ECMP_HASH_USE_RTAG7f);

    if (ecmp_hash_use_rtag7 == 0) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Unit %d - ECMP RH Hash calculation:  non rtag7 calc not supported\n"),
                     unit));
        *hash_value =  0;
        return BCM_E_NONE;
    }

    /* check if is uses flow hash */
    SOC_IF_ERROR_RETURN
        (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel));
    if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_RH_ECMPf)) {
        rtag7_ecmp_use_macro_flow_hash =
            soc_reg_field_get(unit, RTAG7_HASH_SELr,
                              rtag7_hash_sel, USE_FLOW_SEL_RH_ECMPf);
    } else {
        /* A0 bincomp value */
        rtag7_ecmp_use_macro_flow_hash = 0;
    }


    /* RTAG7 hash computation */
    if (rtag7_ecmp_use_macro_flow_hash) {
        SOC_IF_ERROR_RETURN(
            READ_RTAG7_FLOW_BASED_HASHm(unit, MEM_BLOCK_ANY,
                    (hash_Base->rtag7_macro_flow_id & 0xff),
                     &flow_based_hash_entry));    

        /* Generating RTAG7 Macro Flow Based Hash Subsel and Offset for use in RSEL1 Stage */
        /* For future designs this logic can be placed in RSEL1 */
        hash_sub_sel = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                           &flow_based_hash_entry, SUB_SEL_ECMPf);
        hash_offset  = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                           &flow_based_hash_entry, OFFSET_ECMPf);
        concat       = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                           &flow_based_hash_entry, CONCATENATE_HASH_FIELDS_ECMPf);
    } else if (SOC_MEM_IS_VALID(unit, RTAG7_PORT_BASED_HASHm)) {     
        if (hash_Base->dev_src_port < 0) {
            int field_count = 0;
            soc_field_t fields[3];
            uint32 values[3];
            bcm_gport_t gport;
        
            BCM_GPORT_PROXY_SET(gport, 
                hash_Base->src_modid, hash_Base->src_port);
        
            fields[0] = SUB_SEL_RH_ECMPf;
            field_count++;
            fields[1] = OFFSET_RH_ECMPf;
            field_count++;
            fields[2] = CONCATENATE_HASH_FIELDS_RH_ECMPf;
            field_count++;
            BCM_IF_ERROR_RETURN
                (bcm_esw_port_lport_fields_get(unit, gport, 
                                               LPORT_PROFILE_RTAG7_TAB,
                                               field_count, fields, values));
            hash_sub_sel = values[0];
            hash_offset  = values[1];
            concat       = values[2];
        } else {
            port_based_hash_index = hash_Base->dev_src_port + 
                                    soc_mem_index_count(unit, LPORT_TABm);      

            SOC_IF_ERROR_RETURN(     
                READ_RTAG7_PORT_BASED_HASHm(unit, MEM_BLOCK_ANY, 
                            port_based_hash_index, &port_based_hash_entry));

            hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                    &port_based_hash_entry, SUB_SEL_RH_ECMPf);
            hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, OFFSET_RH_ECMPf);
            concat       = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, CONCATENATE_HASH_FIELDS_RH_ECMPf);
        }
    } else {
        hash_sub_sel = 0;
        hash_offset  = 0;
        concat       = 0;
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - ecmp rh hash_seb_sel=%d, hash_offset=%d, concat=%d\n"),
                 unit, hash_sub_sel, hash_offset, concat));

    BCM_IF_ERROR_RETURN
        (select_td2_hash_subfield(concat, hash_sub_sel, &hash_subfield, hash_Base));

    hash_subfield_width = concat ? 64 : 16;

    /* Barrel shift the hash subfield, then take the LSB 16 bits */
    HASH_VALUE_64_COMPUTE(hash_subfield, hash_offset, hash_subfield_width);
    HASH_VALUE_32_COMPUTE(*hash_value, hash_subfield);
    *hash_value &= 0xffff;  
    
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - ecmp rh hash val=%d\n"),
                 unit,  *hash_value));

    return BCM_E_NONE;
    /* ***************** END of ECMP RH HASH CALCULATIONS.  ************************ */
}
#endif /* INCLUDE_L3 */

/*
 * Function:
 *          compute_td2_rtag7_hash_trunk
 * Description:
 *     get LAG hash value from RTAG7_PORT_BASED_HASHm 
 *     if unicast, SUB_SEL_TRUNK_UCf and OFFSET_TRUNK_UCf are used 
 *     if not unicast, SUB_SEL_TRUNK_NONUCf and OFFSET_TRUNK_NONUCf
 *     if flow hash is used, get subsel and offset from RTAG7_FLOW_BASED_HASH 
 * Parameters:
 *          unit - StrataSwitch PCI device unit number (driver internal).
 *          hash_Base - internal data where trunk hash value is storead
 *          hash_value- result param 
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
compute_td2_rtag7_hash_trunk(int unit , bcm_rtag7_base_hash_t *hash_Base,
                         uint32 *hash_value)
{
    uint32 hash_sub_sel;
    uint32 hash_offset;     
    uint32 rtag7_hash_sel; 
    uint32 port_based_hash_index; 
    uint64 hash_subfield; 
    uint32 hash_subfield_width;
    uint32 offset_shift = 0;
    uint32 concat;
    uint8  rtag7_trunk_use_macro_flow_hash_uc    = 0;
    uint8  rtag7_trunk_use_macro_flow_hash_nonuc = 0; 
    rtag7_port_based_hash_entry_t 
           port_based_hash_entry; 
    rtag7_flow_based_hash_entry_t
            flow_based_hash_entry; 
    uint32 hash_control;
    uint32 nuc_trunk_hash_use_rtag7;

    /* check if flow hash is used for UC*/
    SOC_IF_ERROR_RETURN 
        (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel)); 
    if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_TRUNK_UCf)) {
        rtag7_trunk_use_macro_flow_hash_uc =
            soc_reg_field_get(unit, RTAG7_HASH_SELr, 
                              rtag7_hash_sel, USE_FLOW_SEL_TRUNK_UCf);
    } else { 
        rtag7_trunk_use_macro_flow_hash_uc = 0;  
    }
  
    /* check if flow hash is used for NONUC*/
    SOC_IF_ERROR_RETURN 
        (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel)); 
    if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_TRUNK_NONUCf)) {
        rtag7_trunk_use_macro_flow_hash_nonuc = 
            soc_reg_field_get(unit, RTAG7_HASH_SELr, 
                              rtag7_hash_sel, USE_FLOW_SEL_TRUNK_NONUCf);
    } else {
        rtag7_trunk_use_macro_flow_hash_nonuc = 0;  
    }     
        
    /* RTAG7 hash computation */
    if ((rtag7_trunk_use_macro_flow_hash_nonuc && (hash_Base->is_nonuc == 0)) ||
        (rtag7_trunk_use_macro_flow_hash_uc && (hash_Base->is_nonuc != 0))) { 
        SOC_IF_ERROR_RETURN(
            READ_RTAG7_FLOW_BASED_HASHm(unit, MEM_BLOCK_ANY,
                    (hash_Base->rtag7_macro_flow_id & 0xff), 
                     &flow_based_hash_entry)); 
 
        /* in RTAG7_FLOW_BASED_HASHm, using same selection for 
           UC and NONUC */
        hash_sub_sel = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                       &flow_based_hash_entry, SUB_SEL_TRUNKf);
        hash_offset  = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, OFFSET_TRUNKf);
        concat       = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, CONCATENATE_HASH_FIELDS_TRUNKf);
        offset_shift = 0xffff;
    } else if (SOC_MEM_IS_VALID(unit, RTAG7_PORT_BASED_HASHm)) {
        if (hash_Base->dev_src_port < 0) {
            int field_count = 0;
            soc_field_t fields[3];
            uint32 values[3];
            bcm_gport_t gport;

            BCM_GPORT_PROXY_SET(gport, 
                hash_Base->src_modid, hash_Base->src_port);

            if (hash_Base->is_nonuc) {
                fields[0] = SUB_SEL_TRUNK_NONUCf ;
                field_count++;
                fields[1] = OFFSET_TRUNK_NONUCf;
                field_count++;
                fields[2] = CONCATENATE_HASH_FIELDS_TRUNK_NONUCf;
                field_count++;
                offset_shift = 0xff;
            } else {
                fields[0] = SUB_SEL_TRUNK_UCf ;
                field_count++;
                fields[1] = OFFSET_TRUNK_UCf;
                field_count++;
                fields[2] = CONCATENATE_HASH_FIELDS_TRUNK_UCf;
                field_count++;
                offset_shift = 0x3ff;
            }
            BCM_IF_ERROR_RETURN
                (bcm_esw_port_lport_fields_get(unit, gport, 
                                               LPORT_PROFILE_RTAG7_TAB,
                                               field_count, fields, values));
            hash_sub_sel = values[0];
            hash_offset  = values[1];
            concat       = values[2];
        } else {
            port_based_hash_index = hash_Base->dev_src_port + 
                                    soc_mem_index_count(unit, LPORT_TABm);
            SOC_IF_ERROR_RETURN( 
                READ_RTAG7_PORT_BASED_HASHm(unit, MEM_BLOCK_ANY, 
                            port_based_hash_index, &port_based_hash_entry));
            if (hash_Base->is_nonuc) {
                hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                    &port_based_hash_entry, SUB_SEL_TRUNK_NONUCf);
                hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, OFFSET_TRUNK_NONUCf); 
                concat       = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, CONCATENATE_HASH_FIELDS_TRUNK_NONUCf); 
                offset_shift = 0xff; 
            } else {
                hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, SUB_SEL_TRUNK_UCf); 
                hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, OFFSET_TRUNK_UCf);
                concat       = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, CONCATENATE_HASH_FIELDS_TRUNK_UCf);
                offset_shift = 0x3ff; 
            }
        }
    } else {
        hash_sub_sel = 0;
        hash_offset  = 0; 
        concat       = 0;
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - Trunk hash_seb_sel=%d, hash_offset=%d, concat=%d\n"),
                 unit, hash_sub_sel, hash_offset, concat));

    BCM_IF_ERROR_RETURN
        (select_td2_hash_subfield(concat, hash_sub_sel, &hash_subfield, hash_Base)); 

    hash_subfield_width = concat ? 64 : 16;

    /* Barrel shift the hash subfield, then take the LSB 10 bits for UC and 8 bits for nonUC */
    HASH_VALUE_64_COMPUTE(hash_subfield, hash_offset, hash_subfield_width);
    HASH_VALUE_32_COMPUTE(*hash_value, hash_subfield);
    *hash_value &= offset_shift;  
    
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - Trunk hash_value=%d\n"),
                 unit, *hash_value));

    BCM_IF_ERROR_RETURN
        (READ_HASH_CONTROLr(unit, &hash_control));
    nuc_trunk_hash_use_rtag7 =
        soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                          NON_UC_TRUNK_HASH_USE_RTAG7f);

    if (hash_Base->is_nonuc && nuc_trunk_hash_use_rtag7 == 0) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Unit %d - NonUC trunk Hash calculation:  non rtag7 calc not supported\n"),
                     unit));
        *hash_value = 0;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *          compute_td2_rtag7_hash_hg_trunk
 * Description:
 *     get HGT hash value from RTAG7_PORT_BASED_HASHm 
 *     if unicast, SUB_SEL_TRUNK_UCf and OFFSET_TRUNK_UCf are used 
 *     if not unicast, SUB_SEL_TRUNK_NONUCf and OFFSET_TRUNK_NONUCf
 *     if flow hash is used, get subsel and offset from RTAG7_FLOW_BASED_HASH 
 * Parameters:
 *          unit - StrataSwitch PCI device unit number (driver internal).
 *          hash_Base - internal data where trunk hash value is storead
 *          hash_value- result param 
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
compute_td2_rtag7_hash_hg_trunk(int unit , bcm_rtag7_base_hash_t *hash_Base,
                         uint32 *hash_value)
{
    uint32 hash_sub_sel;
    uint32 hash_offset;     
    uint32 rtag7_hash_sel; 
    uint32 port_based_hash_index; 
    uint64 hash_subfield; 
    uint32 hash_subfield_width;
    uint32 offset_shift = 0;
    uint32 concat;
    uint8  rtag7_hgt_use_macro_flow_hash_uc    = 0;
    uint8  rtag7_hgt_use_macro_flow_hash_nonuc = 0; 
    rtag7_port_based_hash_entry_t 
           port_based_hash_entry; 
    rtag7_flow_based_hash_entry_t
            flow_based_hash_entry; 

    /* check if flow hash is used for UC*/
    SOC_IF_ERROR_RETURN 
        (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel)); 
    if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_HG_TRUNK_UCf)) {
        rtag7_hgt_use_macro_flow_hash_uc =
            soc_reg_field_get(unit, RTAG7_HASH_SELr, 
                              rtag7_hash_sel, USE_FLOW_SEL_HG_TRUNK_UCf);
    } else { 
        rtag7_hgt_use_macro_flow_hash_uc = 0;  
    }
  
    /* check if flow hash is used for NONUC*/
    if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_HG_TRUNK_NONUCf)) {
        rtag7_hgt_use_macro_flow_hash_nonuc = 
            soc_reg_field_get(unit, RTAG7_HASH_SELr, 
                              rtag7_hash_sel, USE_FLOW_SEL_HG_TRUNK_NONUCf);
    } else {
        rtag7_hgt_use_macro_flow_hash_nonuc = 0;  
    }     
        
    /* RTAG7 hash computation */
    if ((rtag7_hgt_use_macro_flow_hash_nonuc && (hash_Base->is_nonuc == 0)) ||
        (rtag7_hgt_use_macro_flow_hash_uc && (hash_Base->is_nonuc != 0))) { 
        SOC_IF_ERROR_RETURN(
            READ_RTAG7_FLOW_BASED_HASHm(unit, MEM_BLOCK_ANY,
                    (hash_Base->rtag7_macro_flow_id & 0xff), 
                     &flow_based_hash_entry)); 
 
        /* in RTAG7_FLOW_BASED_HASHm, using same selection for 
           UC and NONUC */
        hash_sub_sel = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                       &flow_based_hash_entry, SUB_SEL_HG_TRUNKf);
        hash_offset  = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, OFFSET_HG_TRUNKf);
        concat       = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, CONCATENATE_HASH_FIELDS_HG_TRUNKf);
        offset_shift = 0xffff;
    } else if (SOC_MEM_IS_VALID(unit, RTAG7_PORT_BASED_HASHm)) {
        if (hash_Base->dev_src_port < 0) {
            int field_count = 0;
            soc_field_t fields[3];
            uint32 values[3];
            bcm_gport_t gport;

            BCM_GPORT_PROXY_SET(gport, 
                hash_Base->src_modid, hash_Base->src_port);

            if (hash_Base->is_nonuc) {
                fields[0] = SUB_SEL_HG_TRUNK_NONUCf ;
                field_count++;
                fields[1] = OFFSET_HG_TRUNK_NONUCf;
                field_count++;
                fields[2] = CONCATENATE_HASH_FIELDS_HG_TRUNK_NONUCf;
                field_count++;
                offset_shift = 0xff;
            } else {
                fields[0] = SUB_SEL_HG_TRUNK_UCf ;
                field_count++;
                fields[1] = OFFSET_HG_TRUNK_UCf;
                field_count++;
                fields[2] = CONCATENATE_HASH_FIELDS_HG_TRUNK_UCf;
                field_count++;
                offset_shift = 0x3ff;
            }
            BCM_IF_ERROR_RETURN
                (bcm_esw_port_lport_fields_get(unit, gport, 
                                               LPORT_PROFILE_RTAG7_TAB,
                                               field_count, fields, values));
            hash_sub_sel = values[0];
            hash_offset  = values[1];
            concat       = values[2];
        } else {
            port_based_hash_index = hash_Base->dev_src_port + 
                                    soc_mem_index_count(unit, LPORT_TABm);
            SOC_IF_ERROR_RETURN( 
                READ_RTAG7_PORT_BASED_HASHm(unit, MEM_BLOCK_ANY, 
                            port_based_hash_index, &port_based_hash_entry));
            if (hash_Base->is_nonuc) {
                hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                    &port_based_hash_entry, SUB_SEL_HG_TRUNK_NONUCf);
                hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, OFFSET_HG_TRUNK_NONUCf); 
                concat       = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, CONCATENATE_HASH_FIELDS_HG_TRUNK_NONUCf); 
                offset_shift = 0xff; 
            } else {
                hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, SUB_SEL_HG_TRUNK_UCf); 
                hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, OFFSET_HG_TRUNK_UCf);
                concat       = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                    &port_based_hash_entry, CONCATENATE_HASH_FIELDS_HG_TRUNK_UCf);
                offset_shift = 0x3ff; 
            }
        }
    } else {
        hash_sub_sel = 0;
        hash_offset  = 0; 
        concat       = 0;
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - Trunk hash_seb_sel=%d, hash_offset=%d, concat=%d\n"),
                 unit, hash_sub_sel, hash_offset, concat));

    BCM_IF_ERROR_RETURN
        (select_td2_hash_subfield(concat, hash_sub_sel, &hash_subfield, hash_Base)); 

    hash_subfield_width = concat ? 64 : 16;

    /* Barrel shift the hash subfield, then take the LSB 10 bits for UC and 8 bits for nonUC */
    HASH_VALUE_64_COMPUTE(hash_subfield, hash_offset, hash_subfield_width);
    HASH_VALUE_32_COMPUTE(*hash_value, hash_subfield);
    *hash_value &= offset_shift;  
    
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - HG Trunk hash_value=%d\n"),
                 unit, *hash_value));

    return BCM_E_NONE;
}

/*
 * Function:
 *          compute_td2_rtag7_hash_rh_trunk
 * Description:
 *     get LAG RH hash value using SUB_SEL_RH_LAGf and OFFSET_RH_LAGf 
 *     from RTAG7_PORT_BASED_HASHm 
 * Parameters:
 *          unit - StrataSwitch PCI device unit number (driver internal).
 *          hash_Base - internal data where rh hash value is stored
 *          hash_value- result param 
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
compute_td2_rtag7_hash_rh_trunk(int unit , bcm_rtag7_base_hash_t *hash_Base,
                         uint32 *hash_value)
{
    uint32 hash_sub_sel;
    uint32 hash_offset;    
    uint32 rtag7_hash_sel; 
    uint32 port_based_hash_index;
    uint64 hash_subfield;
    uint32 hash_subfield_width;
    uint32 concat;
    uint32 offset_shift = 0;
    rtag7_port_based_hash_entry_t 
           port_based_hash_entry;
    rtag7_flow_based_hash_entry_t
            flow_based_hash_entry; 
    uint32 hash_control;
    uint32 nuc_trunk_hash_use_rtag7;


    /* check if flow hash is used for UC*/
    SOC_IF_ERROR_RETURN 
        (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel)); 
    /* RTAG7 hash computation */
    if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_RH_LAGf) && 
        soc_reg_field_get(unit, RTAG7_HASH_SELr, 
                              rtag7_hash_sel, USE_FLOW_SEL_RH_LAGf)) { 
        SOC_IF_ERROR_RETURN(
            READ_RTAG7_FLOW_BASED_HASHm(unit, MEM_BLOCK_ANY,
                    (hash_Base->rtag7_macro_flow_id & 0xff), 
                     &flow_based_hash_entry)); 
 
        /* in RTAG7_FLOW_BASED_HASHm, using same selection for 
           UC and NONUC */
        hash_sub_sel = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                       &flow_based_hash_entry, SUB_SEL_TRUNKf);
        hash_offset  = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, OFFSET_TRUNKf);
        concat       = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, CONCATENATE_HASH_FIELDS_TRUNKf);
        offset_shift = 0xffff;
    } else if (SOC_MEM_IS_VALID(unit, RTAG7_PORT_BASED_HASHm)) {
        if (hash_Base->dev_src_port < 0) {
            int field_count = 0;
            soc_field_t fields[3];
            uint32 values[3];
            bcm_gport_t gport;

            BCM_GPORT_PROXY_SET(gport, 
                hash_Base->src_modid, hash_Base->src_port);

            fields[0] = SUB_SEL_RH_LAGf ;
            field_count++;
            fields[1] = OFFSET_RH_LAGf;
            field_count++;
            fields[2] = CONCATENATE_HASH_FIELDS_RH_LAGf;
            field_count++;

            BCM_IF_ERROR_RETURN
                (bcm_esw_port_lport_fields_get(unit, gport, 
                                               LPORT_PROFILE_RTAG7_TAB,
                                               field_count, fields, values));
            hash_sub_sel = values[0];
            hash_offset  = values[1];
            concat       = values[2];
            offset_shift = 0x3ff;
        } else {
            port_based_hash_index = hash_Base->dev_src_port + 
                                    soc_mem_index_count(unit, LPORT_TABm);      
            BCM_IF_ERROR_RETURN (  
                READ_RTAG7_PORT_BASED_HASHm(unit, MEM_BLOCK_ANY, 
                                port_based_hash_index, &port_based_hash_entry));
            hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                        &port_based_hash_entry, SUB_SEL_RH_LAGf);
            hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                        &port_based_hash_entry, OFFSET_RH_LAGf);
            concat       = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                        &port_based_hash_entry, CONCATENATE_HASH_FIELDS_RH_LAGf);
            offset_shift = 0x3ff;
        }
    } else {
        hash_sub_sel = 0;
        hash_offset  = 0; 
        concat       = 0;
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - Trunk RH hash_sub_sel=%d, hash_offset=%d, concat=%d\n"),
                 unit, hash_sub_sel, hash_offset, concat));

    BCM_IF_ERROR_RETURN
        (select_td2_hash_subfield(concat, hash_sub_sel, &hash_subfield, hash_Base));

    hash_subfield_width = concat ? 64 : 16;

    /* Barrel shift the hash subfield, then take the LSB 10 bits for UC and 8 bits for nonUC */
    HASH_VALUE_64_COMPUTE(hash_subfield, hash_offset, hash_subfield_width);
    HASH_VALUE_32_COMPUTE(*hash_value, hash_subfield);
    *hash_value &= offset_shift;  

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - Trunk RH hash_value=%d\n"),
                 unit, *hash_value));

    BCM_IF_ERROR_RETURN
        (READ_HASH_CONTROLr(unit, &hash_control));
    nuc_trunk_hash_use_rtag7 =
        soc_reg_field_get(unit, HASH_CONTROLr, hash_control,
                          NON_UC_TRUNK_HASH_USE_RTAG7f);

    if (hash_Base->is_nonuc && nuc_trunk_hash_use_rtag7 == 0) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Unit %d - NonUC trunk Hash calculation:  non rtag7 calc not supported\n"),
                     unit));
        *hash_value = 0;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *          compute_td2_rtag7_hash_rh_hg_trunk
 * Description:
 *     get HGT RH hash value using SUB_SEL_RH_LAGf and OFFSET_RH_LAGf 
 *     from RTAG7_PORT_BASED_HASHm 
 * Parameters:
 *          unit - StrataSwitch PCI device unit number (driver internal).
 *          hash_Base - internal data where rh hash value is stored
 *          hash_value- result param 
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
compute_td2_rtag7_hash_rh_hg_trunk(int unit , bcm_rtag7_base_hash_t *hash_Base,
                         uint32 *hash_value)
{
    uint32 hash_sub_sel;
    uint32 hash_offset;     
    uint32 rtag7_hash_sel; 
    uint32 port_based_hash_index; 
    uint64 hash_subfield; 
    uint32 hash_subfield_width;
    uint32 offset_shift = 0;
    uint32 concat;
    rtag7_port_based_hash_entry_t 
           port_based_hash_entry; 
    rtag7_flow_based_hash_entry_t
            flow_based_hash_entry; 

    /* check if flow hash is used for UC*/
    SOC_IF_ERROR_RETURN 
        (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel)); 
    /* RTAG7 hash computation */
    if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_RH_HGTf) && 
        soc_reg_field_get(unit, RTAG7_HASH_SELr, 
                              rtag7_hash_sel, USE_FLOW_SEL_RH_HGTf)) { 
        SOC_IF_ERROR_RETURN(
            READ_RTAG7_FLOW_BASED_HASHm(unit, MEM_BLOCK_ANY,
                    (hash_Base->rtag7_macro_flow_id & 0xff), 
                     &flow_based_hash_entry)); 
 
        /* in RTAG7_FLOW_BASED_HASHm, using same selection for 
           UC and NONUC */
        hash_sub_sel = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                       &flow_based_hash_entry, SUB_SEL_HG_TRUNKf);
        hash_offset  = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, OFFSET_HG_TRUNKf);
        concat       = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, CONCATENATE_HASH_FIELDS_HG_TRUNKf);
        offset_shift = 0xffff;
    } else if (SOC_MEM_IS_VALID(unit, RTAG7_PORT_BASED_HASHm)) {
        if (hash_Base->dev_src_port < 0) {
            int field_count = 0;
            soc_field_t fields[3];
            uint32 values[3];
            bcm_gport_t gport;

            BCM_GPORT_PROXY_SET(gport, 
                hash_Base->src_modid, hash_Base->src_port);

            fields[0] = SUB_SEL_RH_HGTf ;
            field_count++;
            fields[1] = OFFSET_RH_HGTf;
            field_count++;
            fields[2] = CONCATENATE_HASH_FIELDS_RH_HGTf;
            field_count++;

            BCM_IF_ERROR_RETURN
                (bcm_esw_port_lport_fields_get(unit, gport, 
                                               LPORT_PROFILE_RTAG7_TAB,
                                               field_count, fields, values));
            hash_sub_sel = values[0];
            hash_offset  = values[1];
            concat       = values[2];
            offset_shift = 0x3ff;
        } else {
            port_based_hash_index = hash_Base->dev_src_port + 
                                    soc_mem_index_count(unit, LPORT_TABm);
            SOC_IF_ERROR_RETURN( 
                READ_RTAG7_PORT_BASED_HASHm(unit, MEM_BLOCK_ANY, 
                            port_based_hash_index, &port_based_hash_entry));
            hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                        &port_based_hash_entry, SUB_SEL_RH_HGTf);
            hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                        &port_based_hash_entry, OFFSET_RH_HGTf);
            concat       = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                        &port_based_hash_entry, CONCATENATE_HASH_FIELDS_RH_HGTf);
            offset_shift = 0x3ff;
        }
    } else {
        hash_sub_sel = 0;
        hash_offset  = 0; 
        concat       = 0;
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - HGT RH hash_sub_sel=%d, hash_offset=%d, concat=%d\n"),
                 unit, hash_sub_sel, hash_offset, concat));

    BCM_IF_ERROR_RETURN
        (select_td2_hash_subfield(concat, hash_sub_sel, &hash_subfield, hash_Base)); 

    hash_subfield_width = concat ? 64 : 16;

    /* Barrel shift the hash subfield, then take the LSB 10 bits for UC and 8 bits for nonUC */
    HASH_VALUE_64_COMPUTE(hash_subfield, hash_offset, hash_subfield_width);
    HASH_VALUE_32_COMPUTE(*hash_value, hash_subfield);
    *hash_value &= offset_shift;  
    
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - HGT RH hash_value=%d\n"),
                 unit, *hash_value));

    return BCM_E_NONE;
}

/*
 * Function:
 *      compute_td2_rtag7_hash_dlb_hg_trunk
 * Description:
 *      get HGT DLB hash value using SUB_SEL_DLB_HGTf and OFFSET_DLB_HGTf 
 *      from RTAG7_PORT_BASED_HASHm 
 * Parameters:
 *      unit - StrataSwitch PCI device unit number (driver internal).
 *      hash_Base - internal data where rh hash value is stored
 *      hash_value- result param 
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
compute_td2_rtag7_hash_dlb_hg_trunk(int unit , bcm_rtag7_base_hash_t *hash_Base,
                         uint32 *hash_value)
{
    uint32 hash_sub_sel;
    uint32 hash_offset;     
    uint32 rtag7_hash_sel; 
    uint32 port_based_hash_index; 
    uint64 hash_subfield; 
    uint32 hash_subfield_width;
    uint32 offset_shift = 0;
    uint32 concat;
    rtag7_port_based_hash_entry_t port_based_hash_entry; 
    rtag7_flow_based_hash_entry_t flow_based_hash_entry; 

    /* check if flow hash is used for UC*/
    SOC_IF_ERROR_RETURN 
        (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel)); 
    /* RTAG7 hash computation */
    if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_DLB_HGTf) && 
        soc_reg_field_get(unit, RTAG7_HASH_SELr, 
                              rtag7_hash_sel, USE_FLOW_SEL_DLB_HGTf)) { 
        SOC_IF_ERROR_RETURN(
            READ_RTAG7_FLOW_BASED_HASHm(unit, MEM_BLOCK_ANY,
                    (hash_Base->rtag7_macro_flow_id & 0xff), 
                     &flow_based_hash_entry)); 
 
        /* in RTAG7_FLOW_BASED_HASHm, using same selection for 
           UC and NONUC */
        hash_sub_sel = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                       &flow_based_hash_entry, SUB_SEL_HG_TRUNKf);
        hash_offset  = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, OFFSET_HG_TRUNKf);
        concat       = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, CONCATENATE_HASH_FIELDS_HG_TRUNKf);
        offset_shift = 0xffff;
    } else if (SOC_MEM_IS_VALID(unit, RTAG7_PORT_BASED_HASHm)) {
        port_based_hash_index = hash_Base->dev_src_port + soc_mem_index_count(unit, LPORT_TABm);

        SOC_IF_ERROR_RETURN( 
            READ_RTAG7_PORT_BASED_HASHm(unit, MEM_BLOCK_ANY, 
                        port_based_hash_index, &port_based_hash_entry));
        hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                    &port_based_hash_entry, SUB_SEL_DLB_HGTf);
        hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                    &port_based_hash_entry, OFFSET_DLB_HGTf);
        concat       = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                    &port_based_hash_entry, CONCATENATE_HASH_FIELDS_DLB_HGTf);
        offset_shift = 0x3ff;
    } else {
        hash_sub_sel = 0;
        hash_offset  = 0; 
        concat       = 0;
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - HGT DLB hash_sub_sel=%d, hash_offset=%d, concat=%d\n"),
                 unit, hash_sub_sel, hash_offset, concat));

    BCM_IF_ERROR_RETURN
        (select_td2_hash_subfield(concat, hash_sub_sel, &hash_subfield, hash_Base)); 

    hash_subfield_width = concat ? 64 : 16;

    /* Barrel shift the hash subfield, then take the LSB 10 bits for UC and 8 bits for nonUC */
    HASH_VALUE_64_COMPUTE(hash_subfield, hash_offset, hash_subfield_width);
    HASH_VALUE_32_COMPUTE(*hash_value, hash_subfield);
    *hash_value &= offset_shift;  
    
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - HGT DLB hash_value=%d\n"),
                 unit, *hash_value));

    return BCM_E_NONE;
}

/*
 * Function:
 *          compute_td2_rtag7_vxlan
 * Description: compute entropy value of vxlan packet
 * Parameters:
 *          unit - StrataSwitch PCI device unit number (driver internal).
 *          hash_Base - internal data where trunk hash value is storead
 *          hash_value- result param 
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
compute_td2_rtag7_vxlan(int unit , bcm_rtag7_base_hash_t *hash_Base,
                                   uint32 *hash_value)
{
    uint32 hash_sub_sel;
    uint32 hash_offset;     
    uint32 rtag7_hash_sel; 
    uint32 port_based_hash_index; 
    uint64 hash_subfield; 
    uint32 hash_subfield_width;
    uint32 offset_shift = 0xfffff;
    uint32 concat;
    uint8  rtag7_vxlan_use_flow_sel_entropy_label = 0;
    rtag7_port_based_hash_entry_t 
           port_based_hash_entry; 
    rtag7_flow_based_hash_entry_t
            flow_based_hash_entry; 

    /* check if vxlan use RTAG7_FLOW_BASED_HASH 
       else use RTAG7_PORT_BASED_HASH selection for ENTROPY_LABEL*/
    SOC_IF_ERROR_RETURN 
        (READ_RTAG7_HASH_SELr(unit, &rtag7_hash_sel)); 
    if (soc_reg_field_valid(unit, RTAG7_HASH_SELr, USE_FLOW_SEL_ENTROPY_LABELf)) {
        rtag7_vxlan_use_flow_sel_entropy_label =
            soc_reg_field_get(unit, RTAG7_HASH_SELr, 
                              rtag7_hash_sel, USE_FLOW_SEL_ENTROPY_LABELf);
    } else { 
        rtag7_vxlan_use_flow_sel_entropy_label = 0;  
    }
       
    /* RTAG7 hash computation */
    if (rtag7_vxlan_use_flow_sel_entropy_label) {
        /* read flow_based_hash table and set hash_sub_sel, offset, and concat value */
        SOC_IF_ERROR_RETURN(
            READ_RTAG7_FLOW_BASED_HASHm(unit, MEM_BLOCK_ANY,
                    (hash_Base->rtag7_macro_flow_id & 0xff), 
                     &flow_based_hash_entry)); 
 
       
        hash_sub_sel = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit, 
                                       &flow_based_hash_entry, SUB_SEL_LBID_OR_ENTROPY_LABELf);
        hash_offset  = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, OFFSET_LBID_OR_ENTROPY_LABELf);
        concat       = soc_RTAG7_FLOW_BASED_HASHm_field32_get(unit,
                                       &flow_based_hash_entry, CONCATENATE_HASH_FIELDS_LBID_OR_ENTROPY_LABELf);
    } else if (SOC_MEM_IS_VALID(unit, RTAG7_PORT_BASED_HASHm)) {  
        /* read port_based_hash table and set hash_sub_sel, offset, and concat value */    
        port_based_hash_index = hash_Base->dev_src_port + 
                                    soc_mem_index_count(unit, LPORT_TABm);
        SOC_IF_ERROR_RETURN( 
            READ_RTAG7_PORT_BASED_HASHm(unit, MEM_BLOCK_ANY, 
                        port_based_hash_index, &port_based_hash_entry));
            hash_sub_sel = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit,
                                &port_based_hash_entry, SUB_SEL_ENTROPY_LABELf);
            hash_offset  = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                &port_based_hash_entry, OFFSET_ENTROPY_LABELf); 
            concat       = soc_RTAG7_PORT_BASED_HASHm_field32_get(unit, 
                                &port_based_hash_entry, CONCATENATE_HASH_FIELDS_ENTROPY_LABELf); 
    } else {
        hash_sub_sel = 0;
        hash_offset  = 0; 
        concat       = 0;
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - vxlan hash_seb_sel=%d, hash_offset=%d, concat=%d\n"),
                 unit, hash_sub_sel, hash_offset, concat));
    BCM_IF_ERROR_RETURN
        (select_td2_hash_subfield(concat, hash_sub_sel, &hash_subfield, hash_Base)); 

    hash_subfield_width = concat ? 64 : 16;

    /* Barrel shift the hash subfield, then take the LSB 10 bits for UC and 8 bits for nonUC */
    HASH_VALUE_64_COMPUTE(hash_subfield, hash_offset, hash_subfield_width);
    HASH_VALUE_32_COMPUTE(*hash_value, hash_subfield);
    *hash_value &= offset_shift;  
    
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - vxlan hash_value=%d\n"),
                 unit, *hash_value));

    return BCM_E_NONE;
}


/* check ethertype eligibility for resilient hashing */ 
uint8 check_td2_ether_type_eligibility_for_rh(int unit, uint8 rh_mode, uint32 rh_ethertype) 
{
    uint8  ethertype_enable = 0;
    uint8  ethertype_eligibility_config = 0;
    uint32 ethertype_control;
    uint32 ethercount;
    int    ethertype;  
    rh_ecmp_ethertype_eligibility_map_entry_t ethertype_entry;
    
    if (rh_mode == RTAG7_RH_MODE_ECMP) {
        SOC_IF_ERROR_RETURN
          (READ_RH_ECMP_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, &ethertype_control));

        ethertype_eligibility_config = soc_reg_field_get(unit, 
                                                RH_ECMP_ETHERTYPE_ELIGIBILITY_CONTROLr, 
                                                ethertype_control,
                                                ETHERTYPE_ELIGIBILITY_CONFIGf);
    } else if (rh_mode == RTAG7_RH_MODE_LAG) {
        SOC_IF_ERROR_RETURN
            (READ_RH_LAG_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, &ethertype_control));

        ethertype_eligibility_config = soc_reg_field_get(unit, 
                                                RH_LAG_ETHERTYPE_ELIGIBILITY_CONTROLr, 
                                                ethertype_control,
                                                ETHERTYPE_ELIGIBILITY_CONFIGf);
    } else if (rh_mode == RTAG7_RH_MODE_HGT) {
        SOC_IF_ERROR_RETURN
            (READ_RH_HGT_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, &ethertype_control));

        ethertype_eligibility_config = soc_reg_field_get(unit, 
                                                RH_HGT_ETHERTYPE_ELIGIBILITY_CONTROLr, 
                                                ethertype_control,
                                                ETHERTYPE_ELIGIBILITY_CONFIGf);
    }

    /* Check for 16 Ethertypes supported based on operating in the "exclude mode" or "include mode"*/
    if (ethertype_eligibility_config) {
        ethertype_enable = 0;
        for (ethercount = 0; 
            ethercount <  soc_mem_index_count(unit,RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm); 
            ethercount++) {

            if (rh_mode == RTAG7_RH_MODE_ECMP) {
                SOC_IF_ERROR_RETURN(READ_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                                MEM_BLOCK_ANY, ethercount, &ethertype_entry));
                if (soc_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, VALIDf)) {
                    ethertype = soc_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, ETHERTYPEf);
                    ethertype_enable |= (rh_ethertype == ethertype);
                }
            } else if (rh_mode == RTAG7_RH_MODE_LAG) {
                SOC_IF_ERROR_RETURN(READ_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                                MEM_BLOCK_ANY, ethercount, &ethertype_entry));
                if (soc_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, VALIDf)) {
                    ethertype = soc_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, ETHERTYPEf);
                    ethertype_enable |= (rh_ethertype == ethertype);
                }
            } else if (rh_mode == RTAG7_RH_MODE_HGT) {
                SOC_IF_ERROR_RETURN(READ_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                                MEM_BLOCK_ANY, ethercount, &ethertype_entry));
                if (soc_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, VALIDf)) {
                    ethertype = soc_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, ETHERTYPEf);
                    ethertype_enable |= (rh_ethertype == ethertype);
                }
            }
        } /* end of forloop */
    } else {
        ethertype_enable = 1;
         for (ethercount = 0; 
            ethercount <  soc_mem_index_count(unit,RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm); 
            ethercount++) {

            if (rh_mode == RTAG7_RH_MODE_ECMP) {
                SOC_IF_ERROR_RETURN(READ_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                                MEM_BLOCK_ANY, ethercount, &ethertype_entry));
                if (soc_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, VALIDf)) {
                    ethertype = soc_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, ETHERTYPEf);
                    ethertype_enable &= !(rh_ethertype == ethertype);
                }
            } else if (rh_mode == RTAG7_RH_MODE_LAG) {
                SOC_IF_ERROR_RETURN(READ_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                                MEM_BLOCK_ANY, ethercount, &ethertype_entry));
                if (soc_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, VALIDf)) {
                    ethertype = soc_RH_LAG_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, ETHERTYPEf);
                    ethertype_enable &= !(rh_ethertype == ethertype);
                }
            } else if (rh_mode == RTAG7_RH_MODE_HGT) {
                SOC_IF_ERROR_RETURN(READ_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                                MEM_BLOCK_ANY, ethercount, &ethertype_entry));
                if (soc_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, VALIDf)) {
                    ethertype = soc_RH_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                                &ethertype_entry, ETHERTYPEf);
                    ethertype_enable &= !(rh_ethertype == ethertype);
                }
            }
        } /* end of forloop */
    }

    return ethertype_enable;
}

/* check ethertype eligibility for resilient hashing */ 
uint8 check_td2_ether_type_eligibility_for_dlb(int unit, uint32 dlb_ethertype) 
{
    uint8  ethertype_enable = 0;
    uint8  ethertype_eligibility_config = 0;
    uint32 dlb_hgt_qm_control;
    uint32 ethercount;
    int    ethertype;  
    dlb_hgt_ethertype_eligibility_map_entry_t ethertype_entry;

    SOC_IF_ERROR_RETURN
        (READ_DLB_HGT_QUALITY_MEASURE_CONTROLr(unit, &dlb_hgt_qm_control));
    ethertype_eligibility_config = soc_reg_field_get(unit, 
                                                DLB_HGT_QUALITY_MEASURE_CONTROLr, 
                                                dlb_hgt_qm_control,
                                                ETHERTYPE_ELIGIBILITY_CONFIGf);

    /* Check for 16 Ethertypes supported based on operating 
     * in the "exclude mode" or "include mode". */
    if (ethertype_eligibility_config) {
        ethertype_enable = 0;
        for (ethercount = 0; 
             ethercount < soc_mem_index_count(unit,DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm); 
             ethercount++) {
            SOC_IF_ERROR_RETURN(READ_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                            MEM_BLOCK_ANY, ethercount, &ethertype_entry));
            if (soc_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                            &ethertype_entry, VALIDf)) {
                ethertype = soc_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                            &ethertype_entry, ETHERTYPEf);
                ethertype_enable |= (dlb_ethertype == ethertype);
            }
        }
    } else {
        ethertype_enable = 1;
        for (ethercount = 0; 
             ethercount < soc_mem_index_count(unit,DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm); 
             ethercount++) {
            SOC_IF_ERROR_RETURN(READ_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                            MEM_BLOCK_ANY, ethercount, &ethertype_entry));
            if (soc_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                            &ethertype_entry, VALIDf)) {
                ethertype = soc_DLB_HGT_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                                            &ethertype_entry, ETHERTYPEf);
                ethertype_enable &= !(dlb_ethertype == ethertype);
            }
        }
    }

    return ethertype_enable;
}

/* calcualte resilient hashing value */
int perform_td2_rh(int unit,uint32 flow_set_base, uint8 flow_set_size, uint8 rh_mode, 
                uint32  ecmp_hash,uint32 trunk_hash, 
                uint32* resolved_member, uint8* resolved_member_is_valid) 
{
    uint32 rtag7_hash = 0;
    uint32 flow_set_index = 0;
    uint32 rh_flowset_table_config;
    uint32 port_id;
    uint32 mod_id;
    rh_ecmp_flowset_entry_t ecmp_flow_entry;
    rh_lag_flowset_entry_t  lag_flow_entry;
    rh_hgt_flowset_entry_t  hgt_flow_entry;
   
    if (rh_mode == RTAG7_RH_MODE_ECMP) {
        rtag7_hash = ecmp_hash; 
    } else if (rh_mode == RTAG7_RH_MODE_LAG) {
        rtag7_hash = trunk_hash;
    } else if (rh_mode == RTAG7_RH_MODE_HGT) {
        rtag7_hash = trunk_hash;
    }

    /* Compute flow set index */

    switch (flow_set_size) {
        case 0: /* 0 - Flow Index is  0 */
            flow_set_index = 0;
            break;
        case 1: /* 64 - take 6 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0x3f);
            break;
        case 2: /* 128 - take 7 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0x7f);
            break;
        case 3: /* 256 - take 8 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0xff);
            break;
        case 4: /* 512 - take 9 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0x1ff);
            break;
        case 5: /* 1024 - take 10 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0x3ff);
            break;
        case 6: /* 2048 - take 11 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0x7ff);
            break;
        case 7: /* 4096 - take 12 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0xfff);
            break;
        case 8: /* 8192 - take 13 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0x1fff);
            break;
        case 9: /* 16384 - take 14 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0x3fff);
            break;
        case 10: /* 32768 - take 15 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0x7fff);
            break;
        case 11: /* 65536 - take 16 hash bits*/
            flow_set_index = flow_set_base + (rtag7_hash & 0xffff);
            break;
    }

    if (rh_mode == RTAG7_RH_MODE_ECMP || rh_mode == RTAG7_RH_MODE_LAG) {
        uint32 enhanced_hash_control;
        SOC_IF_ERROR_RETURN(
            READ_ENHANCED_HASHING_CONTROLr(unit, &enhanced_hash_control)); 
        rh_flowset_table_config = soc_reg_field_get(unit, ENHANCED_HASHING_CONTROLr, 
                    enhanced_hash_control, RH_FLOWSET_TABLE_CONFIG_ENCODINGf);
        if (rh_flowset_table_config == 
            ENHANCED_HASHING_CONTROLRH_FLOWSET_TABLE_CONFIG_ENCODING_HALF_AND_HALF) {
        
            /* 32K flow sets are allocated to each ECMP and LAG */
            flow_set_index = flow_set_index & 0x7fff;
        
        } else if (rh_flowset_table_config == 
            ENHANCED_HASHING_CONTROLRH_FLOWSET_TABLE_CONFIG_ENCODING_ALL_ECMP) {
        
            flow_set_index = flow_set_index & 0xffff;
        
        } else if (rh_flowset_table_config == 
            ENHANCED_HASHING_CONTROLRH_FLOWSET_TABLE_CONFIG_ENCODING_ALL_LAG) {
        
            flow_set_index = flow_set_index & 0xffff;
        }
    } else if (rh_mode == RTAG7_RH_MODE_HGT) {
        flow_set_index = flow_set_index & 0xffff;
    }

    if (rh_mode == RTAG7_RH_MODE_ECMP) {
        /* Read RH_ECMP_FLOWSET TABLE */
        SOC_IF_ERROR_RETURN(READ_RH_ECMP_FLOWSETm(unit,
                            MEM_BLOCK_ANY, flow_set_index, &ecmp_flow_entry));
        *resolved_member_is_valid = soc_RH_ECMP_FLOWSETm_field32_get(
                            unit, &ecmp_flow_entry, VALIDf);
        *resolved_member = soc_RH_ECMP_FLOWSETm_field32_get(unit,
                            &ecmp_flow_entry, NEXT_HOP_INDEXf);
    } else if (rh_mode == RTAG7_RH_MODE_LAG) {
        /* Read RH_LAG_FLOWSET TABLE */
        SOC_IF_ERROR_RETURN(READ_RH_LAG_FLOWSETm(unit,
                            MEM_BLOCK_ANY, flow_set_index, &lag_flow_entry));
        *resolved_member_is_valid = soc_RH_LAG_FLOWSETm_field32_get(
                            unit, &lag_flow_entry, VALIDf);
        port_id = soc_RH_LAG_FLOWSETm_field32_get(unit,
                            &lag_flow_entry, PORT_NUMf);

        mod_id =  soc_RH_LAG_FLOWSETm_field32_get(unit,
                            &lag_flow_entry, MODULE_IDf);
        
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Unit %d:RH_LAG_FLOWSET.PORT_NUMf=%d\n"),
                     unit, port_id));
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Unit %d:RH_LAG_FLOWSET.MODULE_IDF=%d\n"),
                     unit, mod_id));

        *resolved_member = port_id | (mod_id << RH_LAG_FLOWSET_MOD_ID_SHIFT_VAL); 
    } else if (rh_mode == RTAG7_RH_MODE_HGT) {
        /* Read RH_HGT_FLOWSET TABLE */
        SOC_IF_ERROR_RETURN(READ_RH_HGT_FLOWSETm(unit,
                            MEM_BLOCK_ANY, flow_set_index, &hgt_flow_entry));
        *resolved_member_is_valid = soc_RH_HGT_FLOWSETm_field32_get(
                            unit, &hgt_flow_entry, VALIDf);
        port_id = soc_RH_HGT_FLOWSETm_field32_get(unit,
                            &lag_flow_entry, EGRESS_PORTf);

        *resolved_member = port_id; 
    }

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d: rh flowset *resolved_member=%d\n"),
                 unit, *resolved_member));

    return BCM_E_NONE;
}


/* calcualte resilient hashing value */
int perform_td2_dlb(int unit, uint32 flow_set_base, uint8 flow_set_size, 
                uint8 hgt_id, uint32 ecmp_hash, uint32 trunk_hash, 
                uint32 *resolved_member, uint8 *resolved_member_is_valid) 
{
    int rv = BCM_E_NONE;
    uint8 port_assign_mode;
    uint32 inact_duration;
    
    int band, member, optimal_candidate = 0;
    uint32 rtag7_hash = trunk_hash;
    uint32 flow_set_index = 0;
    uint32 port_id;

    dlb_hgt_flowset_entry_t dlb_hgt_flowset;
    dlb_hgt_group_control_entry_t dlb_hgt_group_control;
    dlb_hgt_group_membership_entry_t dlb_hgt_group_membership;
    dlb_hgt_member_hw_state_entry_t dlb_hgt_member_hw_state;
    dlb_hgt_member_sw_state_entry_t dlb_hgt_member_sw_state;
    dlb_hgt_member_status_entry_t dlb_hgt_member_status;    
    dlb_hgt_flowset_timestamp_page_entry_t dlb_hgt_flowset_timestamp_page;
    dlb_hgt_member_attribute_entry_t dlb_hgt_member_attribute;

    SHR_BITDCL *hw_link_status = NULL;
    SHR_BITDCL *sw_override_map = NULL;
    SHR_BITDCL *sw_link_status = NULL;
    SHR_BITDCL *dlb_hgt_group_members = NULL;
    SHR_BITDCL *obs_page_bits = NULL;

    uint8 assigned_member_is_present, member_is_present, resolved_member_is_present;
    uint8 assigned_member_is_up, member_is_up, resolved_member_is_up;
    uint8 assigned_member_is_valid, assigned_candidate_is_valid = 0;
    uint8 quality_matched, reassignment_eligible;
    uint32 dlb_hgt_current_time;
    uint32 select_member = 0, assigned_member, assigned_candidate = 0; 
    uint8 final_quality_measure[BCM_PBMP_PORT_MAX];
    bcm_pbmp_t band_member_bitmap[8];
    bcm_pbmp_t optimal_band;

    int total_member_cnt;
    uint32 obs_time, current_time, observation_time, inactive_time, obs_page;
    uint32 alloc_size;

    total_member_cnt = soc_mem_index_count(unit, DLB_HGT_MEMBER_STATUSm);
    
    for (band = 0; band < 8; band++) {
        BCM_PBMP_CLEAR(band_member_bitmap[band]);
    }
    BCM_PBMP_CLEAR(optimal_band);

    for (member = 0; member < BCM_PBMP_PORT_MAX; member++) {
        final_quality_measure[member] = 0;
    }

    SOC_IF_ERROR_RETURN(READ_DLB_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY, 
                        hgt_id, &dlb_hgt_group_control));
    
    port_assign_mode = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit, 
                                &dlb_hgt_group_control, PORT_ASSIGNMENT_MODEf);    
    inact_duration = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit, 
                                &dlb_hgt_group_control, INACTIVITY_DURATIONf); 

    /* Compute flow set index */
    switch (flow_set_size) {
    case 0: /* 0 - Flow Index is  0 */
        flow_set_index = 0;
        break;
    case 1: /* 512 - take 9 hash bits*/
        flow_set_index = (flow_set_base + (rtag7_hash & 0x1ff)) & 0x7fff;
        break;
    case 2: /* 1024 - take 10 hash bits*/
        flow_set_index = (flow_set_base + (rtag7_hash & 0x3ff)) & 0x7fff;
        break;
    case 3: /* 2048 - take 11 hash bits*/
        flow_set_index = (flow_set_base + (rtag7_hash & 0x7ff)) & 0x7fff;
        break;
    case 4: /* 4096 - take 12 hash bits*/
        flow_set_index = (flow_set_base + (rtag7_hash & 0xfff)) & 0x7fff;
        break;
    case 5: /* 8192 - take 13 hash bits*/
        flow_set_index = (flow_set_base + (rtag7_hash & 0x1fff)) & 0x7fff;
        break;
    case 6: /* 16384 - take 14 hash bits*/
        flow_set_index = (flow_set_base + (rtag7_hash & 0x3fff)) & 0x7fff;
        break;
    case 7: /* 32768 - take 15 hash bits*/
        flow_set_index = (flow_set_base + (rtag7_hash & 0x7fff)) & 0x7fff;
        break;
    }
   
    /* Read DLB_HGT_FLOWSET TABLE */
    SOC_IF_ERROR_RETURN(READ_DLB_HGT_FLOWSETm(unit, MEM_BLOCK_ANY, 
                        flow_set_index, &dlb_hgt_flowset));
    assigned_member_is_valid = soc_DLB_HGT_FLOWSETm_field32_get(
                        unit, &dlb_hgt_flowset, VALIDf);
    assigned_member = soc_DLB_HGT_FLOWSETm_field32_get(unit,
                        &dlb_hgt_flowset, MEMBER_IDf);
    
    alloc_size = SHR_BITALLOCSIZE(BCM_PBMP_PORT_MAX);
    dlb_hgt_group_members = sal_alloc(alloc_size, "DLB HGT member bitmap"); 
    if (NULL == dlb_hgt_group_members) {
        return BCM_E_MEMORY;
    }
    sal_memset(dlb_hgt_group_members, 0, alloc_size);

    rv = READ_DLB_HGT_GROUP_MEMBERSHIPm(unit, MEM_BLOCK_ANY, hgt_id, &dlb_hgt_group_membership);
    if (BCM_FAILURE(rv)) {
        goto _exit;
    }
    soc_DLB_HGT_GROUP_MEMBERSHIPm_field_get(unit, &dlb_hgt_group_membership, 
                        MEMBER_BITMAPf, dlb_hgt_group_members);

    hw_link_status = sal_alloc(alloc_size, "HW link status bitmap"); 
    if (NULL == hw_link_status) {
        rv = BCM_E_MEMORY;
        goto _exit;
    }
    sal_memset(hw_link_status, 0, alloc_size); 
    rv = READ_DLB_HGT_MEMBER_HW_STATEm(unit, MEM_BLOCK_ANY, 
                        0, &dlb_hgt_member_hw_state);
    if (BCM_FAILURE(rv)) {
        goto _exit;
    }    
    soc_DLB_HGT_MEMBER_HW_STATEm_field_get(unit, &dlb_hgt_member_hw_state, 
                        BITMAPf, hw_link_status);

    sw_override_map = sal_alloc(alloc_size, "SW link override bitmap"); 
    if (NULL == sw_override_map) {
        rv = BCM_E_MEMORY;
        goto _exit;
    }
    sal_memset(sw_override_map, 0, alloc_size); 

    sw_link_status = sal_alloc(alloc_size, "SW link status bitmap"); 
    if (NULL == sw_link_status) {
        rv = BCM_E_MEMORY;
        goto _exit;
    }
    sal_memset(sw_link_status, 0, alloc_size); 
    
    rv = READ_DLB_HGT_MEMBER_SW_STATEm(unit, MEM_BLOCK_ANY, 
                        0, &dlb_hgt_member_sw_state);
    if (BCM_FAILURE(rv)) {
        goto _exit;
    } 
    
    soc_DLB_HGT_MEMBER_SW_STATEm_field_get(unit, &dlb_hgt_member_sw_state, 
                        OVERRIDE_MEMBER_BITMAPf, sw_override_map);
    soc_DLB_HGT_MEMBER_SW_STATEm_field_get(unit, &dlb_hgt_member_sw_state, 
                        MEMBER_BITMAPf, sw_link_status);    

    assigned_member_is_present = SHR_BITGET(dlb_hgt_group_members, assigned_member);

    if (SHR_BITGET(sw_override_map, assigned_member)) {
        assigned_member_is_up = SHR_BITGET(sw_link_status, assigned_member);
    } else {
        assigned_member_is_up = SHR_BITGET(hw_link_status, assigned_member);
    }

    if (assigned_member_is_present && assigned_member_is_valid && assigned_member_is_up) {
        assigned_candidate = assigned_member;
        assigned_candidate_is_valid = 1;
    }

    /* Per Member Quality Measure Results */
    for (member = 0; member < total_member_cnt; member++) {
        rv = READ_DLB_HGT_MEMBER_STATUSm(unit, MEM_BLOCK_ANY, 
                        member, &dlb_hgt_member_status);
        if (BCM_FAILURE(rv)) {
            goto _exit;
        }         
        final_quality_measure[member] = soc_DLB_HGT_MEMBER_STATUSm_field32_get(
                        unit, &dlb_hgt_member_status, QUALITYf);
    }

    /* Group the ports into one of the 8 quality bands */
    for (band = 0; band < 8; band++) {
        for (member = 0; member < total_member_cnt; member++) {
            member_is_present = SHR_BITGET(dlb_hgt_group_members, member);

            if (SHR_BITGET(sw_override_map, member)) {
                member_is_up = SHR_BITGET(sw_link_status, member);
            } else {
                member_is_up = SHR_BITGET(hw_link_status, member);
            }

            quality_matched = (final_quality_measure[member] == band);

            if (member_is_present && quality_matched && member_is_up) {
                BCM_PBMP_PORT_SET(band_member_bitmap[band], member); 
            }
        }
    }

    /* Pick the optimal quality band */
    if (BCM_PBMP_NOT_NULL(band_member_bitmap[7])) {
        BCM_PBMP_ASSIGN(optimal_band, band_member_bitmap[7]); 
    } else if (BCM_PBMP_NOT_NULL(band_member_bitmap[6])) {
        BCM_PBMP_ASSIGN(optimal_band, band_member_bitmap[6]); 
    } else if (BCM_PBMP_NOT_NULL(band_member_bitmap[5])) {
        BCM_PBMP_ASSIGN(optimal_band, band_member_bitmap[5]); 
    } else if (BCM_PBMP_NOT_NULL(band_member_bitmap[4])) {
        BCM_PBMP_ASSIGN(optimal_band, band_member_bitmap[4]); 
    } else if (BCM_PBMP_NOT_NULL(band_member_bitmap[3])) {
        BCM_PBMP_ASSIGN(optimal_band, band_member_bitmap[3]); 
    } else if (BCM_PBMP_NOT_NULL(band_member_bitmap[2])) {
        BCM_PBMP_ASSIGN(optimal_band, band_member_bitmap[2]); 
    } else if (BCM_PBMP_NOT_NULL(band_member_bitmap[1])) {
        BCM_PBMP_ASSIGN(optimal_band, band_member_bitmap[1]); 
    } else if (BCM_PBMP_NOT_NULL(band_member_bitmap[0])) {
        BCM_PBMP_ASSIGN(optimal_band, band_member_bitmap[0]); 
    }

    /* 
     * Pick the optimal candidate from the optimal quality band - Use LFSR/SEED    
     * - implement an XNOR LFSR!
     */
    for (member = 0; member < total_member_cnt; member++) {
        if (BCM_PBMP_MEMBER(optimal_band, member)) {
            optimal_candidate = member;
        }
    }

    obs_time = soc_DLB_HGT_FLOWSETm_field32_get(unit, &dlb_hgt_flowset, TIMESTAMPf);
    obs_page_bits = sal_alloc(alloc_size, "observation page bitmap"); 
    if (NULL == obs_page_bits) {
        rv = BCM_E_MEMORY;
        goto _exit;
    }
    sal_memset(obs_page_bits, 0, alloc_size); 
    rv = READ_DLB_HGT_FLOWSET_TIMESTAMP_PAGEm(unit, MEM_BLOCK_ANY, 
                    (flow_set_index >> 5), &dlb_hgt_flowset_timestamp_page);
    if (BCM_FAILURE(rv)) {
        goto _exit;
    }    
    soc_DLB_HGT_FLOWSET_TIMESTAMP_PAGEm_field_get(unit, &dlb_hgt_flowset_timestamp_page, 
                                        TIMESTAMP_PAGE_BITSf, obs_page_bits);
    /* get 2 bits page value [flow_set_index & 0x1f) * 2 -> +1 ] */
    obs_page = SHR_BITGET(obs_page_bits, ((flow_set_index & 0x1f) * 2 + 1));
    obs_page = obs_page * 2 + SHR_BITGET(obs_page_bits, ((flow_set_index & 0x1f) * 2));

    rv = READ_DLB_HGT_CURRENT_TIMEr(unit, &dlb_hgt_current_time); 
    if (BCM_FAILURE(rv)) {
        goto _exit;
    }
    current_time = soc_reg_field_get(unit, DLB_HGT_CURRENT_TIMEr, dlb_hgt_current_time, CURRENT_TIMEf);
    observation_time = (obs_page << 12) | (obs_time & 0xfff);
    /* MAX_DLB_TIMESTAMP = 0xfff */
    inactive_time = (current_time > observation_time) ? ((current_time - observation_time) & 0x7fff) : 
                    ((current_time + (4 * 0xfff) - observation_time) & 0x7fff); 

    reassignment_eligible = (inactive_time >= inact_duration) || (assigned_candidate_is_valid == 0);

    /* Pick one of them - assigned candidate or optimal candidate */
    switch (port_assign_mode) {
    case 0: /* Eligibility mode */
        if (reassignment_eligible) {
            select_member = optimal_candidate;
        } else {
            select_member = assigned_candidate;
        }
        break;
    case 1: /* static assignment mode */
        select_member = assigned_candidate;
        break;
    case 2: /* spray mode */
        select_member = optimal_candidate;
        break;
    case 3: /* invalid mode */
        break;
    }

    if (SHR_BITGET(sw_override_map, select_member)) {
        resolved_member_is_up = SHR_BITGET(sw_link_status, select_member);
    } else {
        resolved_member_is_up = SHR_BITGET(hw_link_status, select_member);
    }

    resolved_member_is_present = SHR_BITGET(dlb_hgt_group_members, select_member);
    *resolved_member_is_valid = resolved_member_is_present & resolved_member_is_up;

    rv = READ_DLB_HGT_MEMBER_ATTRIBUTEm(unit, MEM_BLOCK_ANY, select_member, 
                                    &dlb_hgt_member_attribute);
    if (BCM_FAILURE(rv)) {
        goto _exit;
    }
    
    port_id = soc_DLB_HGT_MEMBER_ATTRIBUTEm_field32_get(unit, 
                                    &dlb_hgt_member_attribute, PORT_NUMf) & 0x7f;
    
    *resolved_member = port_id; 

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d: DLB HGT flowset *resolved_member=%d\n"),
                 unit, *resolved_member));

    rv = BCM_E_NONE;
    
_exit:
    if (hw_link_status) {
        sal_free(hw_link_status);
    }

    if (sw_override_map) {
        sal_free(sw_override_map);
    }

    if (sw_link_status) {
        sal_free(sw_link_status);
    }

    if (dlb_hgt_group_members) {
        sal_free(dlb_hgt_group_members);
    }     

    if (obs_page_bits) {
        sal_free(obs_page_bits);
    }
    return rv;

}


#ifdef INCLUDE_L3
/* helper function check if resilient hashing is enabled on an ecmp group */
STATIC uint8 
check_td2_ecmp_rh_enable (int unit, int ecmp_group, uint32 ethertype) 
{
    ecmp_count_entry_t ecmpc_entry;
    uint8  rh_ecmp_ethertype_enable, rh_ecmp_enable=0;
    uint32 rh_flowset_table_config, enhanced_hash_control;

    SOC_IF_ERROR_RETURN(READ_ENHANCED_HASHING_CONTROLr(unit, &enhanced_hash_control));    
    rh_flowset_table_config = soc_reg_field_get(unit, ENHANCED_HASHING_CONTROLr, enhanced_hash_control,
                                                RH_FLOWSET_TABLE_CONFIG_ENCODINGf);


    BCM_IF_ERROR_RETURN
        (READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, ecmp_group, &ecmpc_entry));
    rh_ecmp_enable  = soc_L3_ECMP_COUNTm_field32_get(unit, &ecmpc_entry, ENHANCED_HASHING_ENABLEf);    
    
    rh_ecmp_ethertype_enable = check_td2_ether_type_eligibility_for_rh(unit, RTAG7_RH_MODE_ECMP, ethertype);
    
    if ((rh_flowset_table_config == ENHANCED_HASHING_CONTROLRH_FLOWSET_TABLE_CONFIG_ENCODING_HALF_AND_HALF) ||
        (rh_flowset_table_config == ENHANCED_HASHING_CONTROLRH_FLOWSET_TABLE_CONFIG_ENCODING_ALL_ECMP)) {
        rh_ecmp_enable = (rh_ecmp_enable && rh_ecmp_ethertype_enable);
    }
    return rh_ecmp_enable;
}


/*
 * Function:
 *          get_td2_hash_ecmp
 * Description:
 *     compute ecmp destination among multipath interfaces 
 * Parameters:
 *  unit - StrataSwitch PCI device unit number (driver internal).
 *  ecmp_group - multipath id
 *  hash_index - hash value calculated from compute_td2_ecmp_hash
 *  hash_rh_index -  hash value calculated from compute_td2_ecmp_rh_hash
 *  nh_id - where egress l3 interface id is stored
 *  ecmp_rh_enable - flag indicating if rh is enable for this hashing calculation
 *  ethertype - Ethertype of a packet
 *
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
get_td2_hash_ecmp(int unit, int ecmp_group, uint32 hash_index,uint32 hash_rh_index, bcm_if_t *nh_id,
                  uint8 ecmp_rh_enable, uint32 ethertype)
{
    uint32 ecmp_hash_field_upper_bits_count;
                
    ecmp_count_entry_t ecmpc_entry;
    ecmp_entry_t ecmp_entry;
    initial_l3_ecmp_group_entry_t
                 ecmp_group_entry;
 
    uint32 ecmp_ptr = 0;
    uint32 ecmp_count = 0;
    uint32 hash_control;
                
    uint32 ecmp_mask;
    uint32 ecmp_offset;
    uint32 ecmp_index;
    
    uint32 ecmp_rh_flow_set_base;
    uint8  ecmp_rh_flow_set_size;
    uint32 resolved_member;
    uint8  resolved_member_valid;

      /*register read*/
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
                
    if (ecmp_rh_enable) {
        SOC_IF_ERROR_RETURN
            (READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, ecmp_group, &ecmp_group_entry));    
        ecmp_rh_flow_set_base = soc_L3_ECMP_COUNTm_field32_get(unit, &ecmp_group_entry, RH_FLOW_SET_BASEf);    
        ecmp_rh_flow_set_size = soc_L3_ECMP_COUNTm_field32_get(unit, &ecmp_group_entry, RH_FLOW_SET_SIZEf);    

        perform_td2_rh(unit, ecmp_rh_flow_set_base, ecmp_rh_flow_set_size, RTAG7_RH_MODE_ECMP, hash_rh_index, 0,
                  &resolved_member, &resolved_member_valid);

        if (resolved_member_valid) {
            *nh_id = resolved_member & 0xffff; 
        } else {
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "Unit %d - Hash calculation: Such Configuration is not supported: resolved_lag_member_valid==FALSE\n"),
                         unit));
            return BCM_E_PARAM;
        }
    } else {
        /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
         * A ECMP_COUNT value of 0 implies 1 entry 
         * a value of 1 implies 2 entries etc...
         * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
         */
        SOC_IF_ERROR_RETURN
            (READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, ecmp_group, &ecmpc_entry));
 
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
        
        ecmp_offset = ((hash_index & ecmp_mask) % (ecmp_count + 1)) & 0x3FF;
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tECMP offset 0x%08x, ptr 0x%x\n"),
                     ecmp_offset, ecmp_ptr));
        ecmp_index = (ecmp_ptr + ecmp_offset) & 0xfff;
        SOC_IF_ERROR_RETURN
            (READ_L3_ECMPm(unit, MEM_BLOCK_ANY, ecmp_index, &ecmp_entry));

        *nh_id = 
              soc_L3_ECMPm_field32_get(unit, &ecmp_entry,
                                     NEXT_HOP_INDEXf); 
        *nh_id = *nh_id & 0xffff;
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tECMP next hop HW index 0x%08x\n"),
                     *nh_id));
    }

    return BCM_E_NONE;  
}
#endif /* INCLUDE_L3 */

/* helper function check if resilient hashing is enabled on a lag */
uint8 
check_td2_lag_rh_enable (int unit, int tgid, uint32 ethertype) 
{
    trunk_group_entry_t dst_tg_entry;
    uint8  rh_lag_group_enable, rh_lag_ethertype_enable, lag_rh_enable=0;
    uint32 rh_flowset_table_config, enhanced_hash_control;

    SOC_IF_ERROR_RETURN(READ_ENHANCED_HASHING_CONTROLr(unit, &enhanced_hash_control));    
    rh_flowset_table_config = soc_reg_field_get(unit, ENHANCED_HASHING_CONTROLr, enhanced_hash_control,
                                                RH_FLOWSET_TABLE_CONFIG_ENCODINGf);


    BCM_IF_ERROR_RETURN
        (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tgid, &dst_tg_entry));
    rh_lag_group_enable  = soc_TRUNK_GROUPm_field32_get(unit, &dst_tg_entry, ENHANCED_HASHING_ENABLEf);    
    
    rh_lag_ethertype_enable = check_td2_ether_type_eligibility_for_rh(unit, RTAG7_RH_MODE_LAG, ethertype);
    
    if ((rh_flowset_table_config == ENHANCED_HASHING_CONTROLRH_FLOWSET_TABLE_CONFIG_ENCODING_HALF_AND_HALF) ||
        (rh_flowset_table_config == ENHANCED_HASHING_CONTROLRH_FLOWSET_TABLE_CONFIG_ENCODING_ALL_LAG)) {
        lag_rh_enable = (rh_lag_group_enable && rh_lag_ethertype_enable);
    }
    return lag_rh_enable;
}

/* helper function check if resilient hashing is enabled on a hg trunk */
uint8 
check_td2_hgt_dlb_enable (int unit, int hgt_id, uint32 ethertype) 
{
    dlb_hgt_group_control_entry_t dlb_hgt_grp_ctrl_entry;
    hg_trunk_group_entry_t dst_hgt_entry;
    uint8 dlb_hgt_group_enable, enhanced_hash_enable;
    uint8 dlb_hgt_ethertype_enable, hgt_dlb_enable = 0;

    BCM_IF_ERROR_RETURN
      (READ_DLB_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY, (hgt_id & 0x3f), &dlb_hgt_grp_ctrl_entry));
    dlb_hgt_group_enable  = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit,
                                  &dlb_hgt_grp_ctrl_entry, GROUP_ENABLEf);
    BCM_IF_ERROR_RETURN
        (READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgt_id, &dst_hgt_entry));
    enhanced_hash_enable  = soc_HG_TRUNK_GROUPm_field32_get(unit, &dst_hgt_entry, 
                            ENHANCED_HASHING_ENABLEf);    
    
    dlb_hgt_ethertype_enable = check_td2_ether_type_eligibility_for_dlb(unit, ethertype);
    
    if ((dlb_hgt_group_enable) && (enhanced_hash_enable) && (dlb_hgt_ethertype_enable)) {
        hgt_dlb_enable = (enhanced_hash_enable && dlb_hgt_ethertype_enable);
    }
    
    return hgt_dlb_enable;
}

/* helper function check if resilient hashing is enabled on a lag */
uint8 
check_td2_hgt_rh_enable (int unit, int hgtid, uint32 ethertype) 
{
    hg_trunk_group_entry_t hg_tg_entry;
    uint8  rh_hgt_group_enable, rh_hgt_ethertype_enable, hgt_rh_enable = 0;
    uint32 rh_hgt_enable, enhanced_hash_control;

    SOC_IF_ERROR_RETURN(READ_ENHANCED_HASHING_CONTROLr(unit, &enhanced_hash_control));    
    rh_hgt_enable = soc_reg_field_get(unit, ENHANCED_HASHING_CONTROLr, enhanced_hash_control,
                                                RH_HGT_ENABLEf);
    BCM_IF_ERROR_RETURN
        (READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid, &hg_tg_entry));
    rh_hgt_group_enable  = soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_tg_entry, ENHANCED_HASHING_ENABLEf);    
    
    rh_hgt_ethertype_enable = check_td2_ether_type_eligibility_for_rh(unit, RTAG7_RH_MODE_HGT, ethertype);
    
    hgt_rh_enable = (rh_hgt_enable && rh_hgt_group_enable && rh_hgt_ethertype_enable);

    return hgt_rh_enable;
}

/*
 * Function:
 *          get_td2_hash_trunk
 * Description:
 *     compute destination trunk member port 
 * Parameters:
 *  unit - StrataSwitch PCI device unit number (driver internal).
 *  ecmp_group - multipath id
 *  tgid - trunk port id
 *  hash_index - hash value calculated from compute_td2_rtag7_hash_trunk
 *  hash_rh_index -  hash value calculated from compute_td2_rtag7_hash_rh_trunk
 *  dst_tgid - where egress member port id is stored
 *  hash_rh_value - flag indicating if rh is enable for this hashing calculation
 *  ethertype - Ethertype of a packet
 *
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
get_td2_hash_trunk(int unit, int tgid, uint32 hash_index, 
                bcm_gport_t *dst_tgid, uint32 hash_rh_value, 
                  uint8 lag_rh_enable, uint32 ethertype)
{
    uint32 trunk_base;
    uint32 trunk_group_size;    
    uint32 rtag;
    uint32 trunk_index;
    uint32 trunk_member_table_index;
    bcm_module_t mod_id;
    bcm_port_t port_id;
    int mod_is_local;
    trunk_member_entry_t trunk_member_entry;
    trunk_group_entry_t  tg_entry, dst_tg_entry;
    _bcm_gport_dest_t   dest;
   
    uint32 lag_rh_flow_set_base;
    uint32 lag_rh_flow_set_size;
    
    uint32 resolved_member;
    uint8  resolved_member_valid;
    
    BCM_IF_ERROR_RETURN
        (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tgid, &tg_entry));

    trunk_base           = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, BASE_PTRf);
    trunk_group_size     = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, TG_SIZEf);
    rtag                 = soc_TRUNK_GROUPm_field32_get(unit, &tg_entry, RTAGf);
                 
    if (rtag != 7){
         LOG_VERBOSE(BSL_LS_BCM_COMMON,
                     (BSL_META_U(unit,
                                 "Unit %d - Hash calculation: uport only RTAG7 calc no support for rtag %d\n"),
                      unit, rtag));
    }

    trunk_index = hash_index % (trunk_group_size + 1);
    trunk_member_table_index = (trunk_base + trunk_index) & 0x7ff;
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "\tTrunk HW index 0x%08x\n"),
                 trunk_index));
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "\tTrunk group size 0x%08x\n"),
                 trunk_group_size));

    if (lag_rh_enable) {
        BCM_IF_ERROR_RETURN
            (READ_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, tgid, &dst_tg_entry));
        lag_rh_flow_set_base = soc_TRUNK_GROUPm_field32_get(unit, &dst_tg_entry, RH_FLOW_SET_BASEf);    
        lag_rh_flow_set_size = soc_TRUNK_GROUPm_field32_get(unit, &dst_tg_entry, RH_FLOW_SET_SIZEf);    
   
        perform_td2_rh(unit, lag_rh_flow_set_base, lag_rh_flow_set_size, RTAG7_RH_MODE_LAG, 0, hash_rh_value,
                   &resolved_member, &resolved_member_valid);

        if (resolved_member_valid) { 
            port_id = resolved_member & 0x7f;
            mod_id  = (resolved_member >> RH_LAG_FLOWSET_MOD_ID_SHIFT_VAL) & 0xff;
        } else {
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "Unit %d - Hash calculation: Such Configuration is not supported: \n"),
                         unit));
            return BCM_E_PARAM;
        }
    } else {
        BCM_IF_ERROR_RETURN
            (READ_TRUNK_MEMBERm(unit, MEM_BLOCK_ANY, trunk_member_table_index,
                                        &trunk_member_entry));
        mod_id  = soc_TRUNK_MEMBERm_field32_get(unit, &trunk_member_entry,
                                        MODULE_IDf);
        port_id = soc_TRUNK_MEMBERm_field32_get(unit, &trunk_member_entry,
                                        PORT_NUMf);
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET, mod_id, port_id,
                                        &(dest.modid), &(dest.port)));
    dest.gport_type = _SHR_GPORT_TYPE_MODPORT;

    BCM_IF_ERROR_RETURN
        (_bcm_esw_modid_is_local(unit, dest.modid,
                                 &mod_is_local));
    if (mod_is_local) {
        if (IS_ST_PORT(unit, dest.port)) {
            dest.gport_type = _SHR_GPORT_TYPE_DEVPORT;
        }
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_gport_construct(unit, &dest, dst_tgid));

    return BCM_E_NONE;  
}

/*
 * Function:
 *     get_td2_hash_trunk_nuc
 * Description:
 *     Compute non-unicast destination trunk member port 
 * Parameters:
 *  unit - StrataSwitch PCI device unit number (driver internal).
 *  tgid - Trunk group id
 *  fwd_reason - Flow foward reason. unicast, ipmc, l2mc, broadcast, dlf.
 *  hash_index - Hash value calculated based on RTAG7
 *  dst_gport  - Where egress member port id is stored
 *
 * Returns:
 *      BCM_E_xxxx
 *
 */
STATIC int
get_td2_hash_trunk_nuc(int unit, int tgid, 
                   bcm_switch_pkt_hash_info_fwd_reason_t fwd_reason, 
                   uint32 hash_index, bcm_gport_t *dst_gport)
{
    int nuc_type;
    int nonuc_trunk_block_mask_index;
    int region_size, i, all_local;
    int member_index, member_count;
    int modid, modid_tmp, port, tid, id, mod_is_local;
    int port_index, count;
    bcm_pbmp_t pbmp, local_pbmp;
    bcm_trunk_info_t   t_add_info;
    bcm_trunk_member_t member_array[BCM_TRUNK_MAX_PORTCNT];
    trunk_bitmap_entry_t trunk_bitmap_entry;

    switch(fwd_reason) {
    case bcmSwitchPktHashInfoFwdReasonIpmc:
        nuc_type = TRUNK_BLOCK_MASK_TYPE_IPMC;
        break;
    case bcmSwitchPktHashInfoFwdReasonL2mc:
        nuc_type = TRUNK_BLOCK_MASK_TYPE_L2MC;
        break;
    case bcmSwitchPktHashInfoFwdReasonBcast:
        nuc_type = TRUNK_BLOCK_MASK_TYPE_BCAST;
        break;
    case bcmSwitchPktHashInfoFwdReasonDlf:
        nuc_type = TRUNK_BLOCK_MASK_TYPE_DLF;
        break;
    default:
        return BCM_E_PARAM;
    }
    nonuc_trunk_block_mask_index = (nuc_type << 8) | (hash_index & 0xff);

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - Nonuc-trunk table index = %d\n"), 
                 unit, nonuc_trunk_block_mask_index));
    region_size = soc_mem_index_count(unit, NONUCAST_TRUNK_BLOCK_MASKm) / 4;

    BCM_IF_ERROR_RETURN(
        bcm_esw_trunk_get(unit, tgid, &t_add_info, BCM_TRUNK_MAX_PORTCNT,
            member_array, &member_count));

    BCM_IF_ERROR_RETURN(
        READ_TRUNK_BITMAPm(unit, MEM_BLOCK_ANY, tgid, &trunk_bitmap_entry));
    BCM_PBMP_CLEAR(pbmp);
    BCM_PBMP_CLEAR(local_pbmp);
    soc_mem_pbmp_field_get(unit, TRUNK_BITMAPm, &trunk_bitmap_entry,
                                    TRUNK_BITMAPf, &local_pbmp);
    BCM_PBMP_COUNT(local_pbmp, count);
    all_local = FALSE;
    if (count == member_count) {
        all_local = TRUE;
    }
    if (all_local || member_count > BCM_SWITCH_TRUNK_MAX_PORTCNT) {
        member_index = 
            (nonuc_trunk_block_mask_index % region_size) % member_count;
    } else {
        member_index = 
            (nonuc_trunk_block_mask_index % BCM_XGS3_TRUNK_MAX_PORTCNT) % member_count;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, member_array[member_index].gport,
                                &modid, &port, &tid, &id));
    port_index = 0;
    for (i = 0; i < member_count; i++) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_gport_resolve(unit, member_array[i].gport,
                                &modid_tmp, &port, &tid, &id));
        if (modid_tmp == modid) {
            if (i <= member_index) {
                port_index++;
            }
            BCM_PBMP_PORT_ADD(pbmp, port);
        }
    }

    count = 0;
    BCM_PBMP_ITER(pbmp, port) {
        count++;
        if (count == port_index) {
            break;
        }
    }

    if (count != port_index) {
        return BCM_E_FAIL;
    }
    
    BCM_IF_ERROR_RETURN
        (_bcm_esw_modid_is_local(unit, modid, &mod_is_local));

    if (mod_is_local && IS_ST_PORT(unit, port)) {
        BCM_GPORT_DEVPORT_SET(*dst_gport, unit, port);
    } else {
        BCM_GPORT_MODPORT_SET(*dst_gport, modid, port);
    }

    return BCM_E_NONE;
}

STATIC int
get_td2_hash_hg_trunk(int unit, int hgtid, uint32 hash_index, 
                bcm_gport_t *dst_tgid, uint32 hash_rh_value, 
                uint32 hash_dlb_value, uint8 lag_rh_enable,
                uint8 lag_dlb_enable, uint32 ethertype)
{
    uint32 trunk_base;
    uint32 trunk_group_size;    
    uint32 rtag;
    uint32 trunk_index;
    uint32 trunk_member_table_index;
    bcm_module_t mod_id;
    bcm_port_t port_id;
    hg_trunk_member_entry_t hg_trunk_member_entry;
    hg_trunk_group_entry_t  hg_tg_entry;
    rh_hgt_group_control_entry_t rh_hgt_group_control_entry;
    dlb_hgt_group_control_entry_t dlb_hgt_group_control_entry;
    _bcm_gport_dest_t   dest;
   
    uint32 hgt_rh_flow_set_base;
    uint32 hgt_rh_flow_set_size;

    uint32 hgt_dlb_flow_set_base;
    uint32 hgt_dlb_flow_set_size;
    
    uint32 resolved_member;
    uint8  resolved_member_valid;
    
    BCM_IF_ERROR_RETURN
        (READ_HG_TRUNK_GROUPm(unit, MEM_BLOCK_ANY, hgtid, &hg_tg_entry));

    trunk_base           = soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_tg_entry, BASE_PTRf);
    trunk_group_size     = soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_tg_entry, TG_SIZEf);
    rtag                 = soc_HG_TRUNK_GROUPm_field32_get(unit, &hg_tg_entry, RTAGf);
                 
    if (rtag != 7) {
         LOG_VERBOSE(BSL_LS_BCM_COMMON,
                     (BSL_META_U(unit,
                                 "Unit %d - Hash calculation: uport only RTAG7 calc no support for rtag %d\n"),
                      unit, rtag));
    }

    trunk_index = hash_index % (trunk_group_size + 1);
    trunk_member_table_index = (trunk_base + trunk_index) & 0xff;
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "\tHG Trunk HW index 0x%08x\n"),
                 trunk_index));
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "\tHG Trunk group size 0x%08x\n"),
                 trunk_group_size));

    if (lag_rh_enable) {
        BCM_IF_ERROR_RETURN
            (READ_RH_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY, hgtid, 
                                        &rh_hgt_group_control_entry));
        hgt_rh_flow_set_base = soc_RH_HGT_GROUP_CONTROLm_field32_get(unit, 
                                        &rh_hgt_group_control_entry, RH_FLOW_SET_BASEf);    
        hgt_rh_flow_set_size = soc_RH_HGT_GROUP_CONTROLm_field32_get(unit, 
                                        &rh_hgt_group_control_entry, RH_FLOW_SET_SIZEf);    
   
        perform_td2_rh(unit, hgt_rh_flow_set_base, hgt_rh_flow_set_size, RTAG7_RH_MODE_HGT, 0, hash_rh_value,
                   &resolved_member, &resolved_member_valid);

        if (resolved_member_valid) { 
            port_id = resolved_member & 0x7f;
        } else {
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "Unit %d - Hash calculation: Such Configuration is not supported: \n"),
                         unit));
            return BCM_E_PARAM;
        }
    } else if (lag_dlb_enable) {
        BCM_IF_ERROR_RETURN
            (READ_DLB_HGT_GROUP_CONTROLm(unit, MEM_BLOCK_ANY, hgtid, 
                                        &dlb_hgt_group_control_entry));
        hgt_dlb_flow_set_base = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit, 
                                        &dlb_hgt_group_control_entry, FLOW_SET_BASEf);    
        hgt_dlb_flow_set_size = soc_DLB_HGT_GROUP_CONTROLm_field32_get(unit, 
                                        &dlb_hgt_group_control_entry, FLOW_SET_SIZEf);    
   
        perform_td2_dlb(unit, hgt_dlb_flow_set_base, hgt_dlb_flow_set_size, hgtid, 0, hash_dlb_value,
                   &resolved_member, &resolved_member_valid);

        if (resolved_member_valid) { 
            port_id = resolved_member & 0x7f;
        } else {
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "Unit %d - Hash calculation: Such Configuration is not supported: \n"),
                         unit));
            return BCM_E_PARAM;
        }
    } else {
        BCM_IF_ERROR_RETURN
            (READ_HG_TRUNK_MEMBERm(unit, MEM_BLOCK_ANY, trunk_member_table_index,
                                        &hg_trunk_member_entry));
        port_id = soc_HG_TRUNK_MEMBERm_field32_get(unit, &hg_trunk_member_entry,
                                        PORT_NUMf);
    }

    if (BCM_FAILURE(bcm_esw_stk_my_modid_get(unit, &mod_id))) {
        mod_id = 0;
    }
    
    BCM_IF_ERROR_RETURN
        (_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET, mod_id, port_id,
                                        &(dest.modid), &(dest.port)));
    dest.gport_type = _SHR_GPORT_TYPE_DEVPORT;
    BCM_IF_ERROR_RETURN(_bcm_esw_gport_construct(unit, &dest, dst_tgid));

    return BCM_E_NONE;  
}

/*
 * Function:
 *          _bcm_td2_switch_pkt_info_hash_get
 * Description:
 *    main td2 hash get function  
 *
 * Parameters     
 *  unit - StrataSwitch PCI device unit number (driver internal).
 *  pkt_info - struct that carries packet's l2, l3, and l4 information
 *  dst_gport - hashing resolved trunk member port as a return val
 *  dst_intf  - hashing resolved ecmp  member port as a retrun val
 *              or entropy label of vxlan as a retrun val
 * Returns:
 *      BCM_E_xxxx
 *
 */
int
_bcm_td2_switch_pkt_info_hash_get(int unit,
                                   bcm_switch_pkt_info_t *pkt_info,
                                   bcm_gport_t *dst_gport,
                                   bcm_if_t *dst_intf)
{
    bcm_rtag7_base_hash_t hash_res;
    int pc_out, mod_is_local;
    bcm_gport_t     port;
    bcm_trunk_t     tid;
    int             id;
    uint32          hash_value;
    uint32          hash_rh_value;
    uint32          hash_dlb_value;
    bcm_ethertype_t ethertype;
    int             rc;

    if (pkt_info == NULL) {
        return BCM_E_PARAM;
    }

    if (!_BCM_SWITCH_PKT_INFO_FLAG_TEST(pkt_info, SRC_GPORT)) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "Unit %d - Hash calculation: source gport value missing\n"),
                     unit));
        return BCM_E_PARAM;
    }

    rc = _bcm_esw_gport_resolve(unit, pkt_info->src_gport,
                                &(hash_res.src_modid),
                                &(hash_res.src_port), 
                                &tid, &id);
    if (rc != BCM_E_NONE) {
        return rc;
    }

    if ((-1 != id) || (-1 != tid)) {
        /* Must be single system port */
        return BCM_E_PORT;
    }

    /* Load balancing retrieval */
    BCM_IF_ERROR_RETURN(
        _bcm_esw_modid_is_local(unit, hash_res.src_modid, &mod_is_local));
    if (mod_is_local) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_port_local_get(unit, pkt_info->src_gport, &port));
        hash_res.dev_src_port = port;
    } else {
        hash_res.dev_src_port = -1;
        if (BCM_GPORT_IS_PROXY(pkt_info->src_gport)) {
            port = pkt_info->src_gport;
        } else {
            BCM_GPORT_PROXY_SET(port,
                hash_res.src_modid, hash_res.src_port);
        }
    }
    BCM_IF_ERROR_RETURN
        (bcm_esw_port_control_get(unit, port,
                                bcmPortControlLoadBalancingNumber,
                                &pc_out)); 
    hash_res.rtag7_port_lbn = pc_out;

    /* check if the foward reason of packet is all non-uc packet or bit 40 in the mac DA is one. */
    hash_res.is_nonuc = pkt_info->fwd_reason || BCM_MAC_IS_MCAST(pkt_info->dst_mac);
    
    BCM_IF_ERROR_RETURN
        (main_td2_do_rtag7_hashing(unit, pkt_info, &hash_res));
    BCM_IF_ERROR_RETURN
        (main_td2_compute_lbid(unit, &hash_res));

    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "Unit %d - Hash status: \n"),
                 unit));
    if (hash_res.hash_a_valid) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tRTAG7 A0 0x%08x\n"),
                     hash_res.rtag7_hash16_value_a_0));
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tRTAG7 A1 0x%08x\n"),
                     hash_res.rtag7_hash16_value_a_1));
    } else {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tRTAG7 A hashes invalid due to missing packet info\n")));
    }
    if (hash_res.hash_b_valid) {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tRTAG7 B0 0x%08x\n"),
                     hash_res.rtag7_hash16_value_b_0));
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tRTAG7 B1 0x%08x\n"),
                     hash_res.rtag7_hash16_value_b_1));
    } else {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tRTAG7 B hashes invalid due to missing packet info\n")));
    }
    LOG_VERBOSE(BSL_LS_BCM_COMMON,
                (BSL_META_U(unit,
                            "\tRTAG7 LBN 0x%08x\n"),
                 hash_res.rtag7_port_lbn));
    if (hash_res.lbid_hash_valid){
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tRTAG7 LBID 0x%08x\n"),
                     hash_res.rtag7_lbid_hash));
    } else {
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tRTAG7 LBID not valid due to non-RTAG7 configuration\n")));
    }

    ethertype = pkt_info->ethertype;

#ifdef INCLUDE_L3
    if (0 != (pkt_info->flags & BCM_SWITCH_PKT_INFO_HASH_MULTIPATH)) {
        bcm_if_t        nh_id;
        uint8           ecmp_rh_enable;
        nh_id = 0;

        if (dst_intf == NULL) {
            return BCM_E_PARAM;
        }

        rc = compute_td2_ecmp_hash(unit, &hash_res, &hash_value);
        if (rc != BCM_E_NONE) {
            return rc;
        }

        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tECMP Hash value 0x%08x\n"),
                     hash_value));
        
        if (0 != (check_td2_ecmp_rh_enable(unit, 
                    pkt_info->mpintf - BCM_XGS3_MPATH_EGRESS_IDX_MIN, ethertype ))) { 
            ecmp_rh_enable = TRUE;
            rc = 
                compute_td2_ecmp_rh_hash(unit, &hash_res, &hash_rh_value);

                if (rc != BCM_E_NONE) {
                    return rc;
                }
        } else {
            ecmp_rh_enable = FALSE;
            hash_rh_value = 0;
        }

   
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tECMP Hash rh value 0x%08x\n"),
                     hash_rh_value));

        rc = get_td2_hash_ecmp(unit,
                           pkt_info->mpintf - BCM_XGS3_MPATH_EGRESS_IDX_MIN,
                           hash_value, hash_rh_value,&nh_id,
                           ecmp_rh_enable, ethertype); 
        /* Convert to egress object format */
        *dst_intf = nh_id + BCM_XGS3_EGRESS_IDX_MIN;
    } else
#endif /* INCLUDE_L3 */
    if (0 != (pkt_info->flags & BCM_SWITCH_PKT_INFO_HASH_TRUNK)) {
        int member_count;
        bcm_trunk_t trunk;
        uint8 lag_rh_enable = FALSE;
        uint8 hgt_rh_enable = FALSE;
        uint8 hgt_dlb_enable = FALSE;
        bcm_trunk_chip_info_t   ti;

        if (dst_gport == NULL) {
            return BCM_E_PARAM;
        }

        trunk = BCM_TRUNK_INVALID;
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

        BCM_IF_ERROR_RETURN(bcm_esw_trunk_chip_info_get(unit, &ti));

        if (trunk >= ti.trunk_id_min && trunk <= ti.trunk_id_max) {
            rc = compute_td2_rtag7_hash_trunk(unit, &hash_res, &hash_value);
            
            if (rc != BCM_E_NONE) {
                return rc;
            }
            
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "\tTrunk Hash value 0x%08x\n"),
                         hash_value));
            
            if (0 != (check_td2_lag_rh_enable(unit, trunk, 
                      ethertype))) { 
                 lag_rh_enable = TRUE;
                 rc = 
                    compute_td2_rtag7_hash_rh_trunk(unit, &hash_res, &hash_rh_value);
            
                    if (rc != BCM_E_NONE) {
                        return rc;
                    }
            } else {
                lag_rh_enable = FALSE;    
                hash_rh_value = 0;
            }
            
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "\tTrunk RH Hash value 0x%08x\n"),
                         hash_rh_value));

            if (hash_res.is_nonuc) {
                BCM_IF_ERROR_RETURN
                    (get_td2_hash_trunk_nuc(unit, trunk, pkt_info->fwd_reason, 
                                lag_rh_enable ? hash_rh_value : hash_value, dst_gport));
            } else {
                BCM_IF_ERROR_RETURN
                    (get_td2_hash_trunk(unit, trunk,
                                        hash_value, dst_gport, hash_rh_value,
                                        lag_rh_enable, ethertype));
            }
        } else if (trunk >= ti.trunk_fabric_id_min && trunk <= ti.trunk_fabric_id_max) {

            rc = compute_td2_rtag7_hash_hg_trunk(unit, &hash_res, &hash_value);
            if (rc != BCM_E_NONE) {
                return rc;
            }
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "\tHG-Trunk Hash value 0x%08x\n"),
                         hash_value));

            if (0 != (check_td2_hgt_rh_enable(unit, trunk - ti.trunk_fabric_id_min, 
                      ethertype))) { 
                hgt_rh_enable = TRUE;
                rc = compute_td2_rtag7_hash_rh_hg_trunk(unit, &hash_res, &hash_rh_value);

                if (rc != BCM_E_NONE) {
                    return rc;
                }
            } else {
                hgt_rh_enable = FALSE;    
                hash_rh_value = 0;
            }

            if (0 != (check_td2_hgt_dlb_enable(unit, trunk - ti.trunk_fabric_id_min, ethertype))) { 
                hgt_dlb_enable = TRUE;
                rc = compute_td2_rtag7_hash_dlb_hg_trunk(unit, &hash_res, &hash_dlb_value);

                if (rc != BCM_E_NONE) {
                    return rc;
                }
            } else {
                hgt_dlb_enable = FALSE;
                hash_dlb_value = 0;
            }

            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "\tHG-Trunk RH Hash value 0x%08x\n"),
                         hash_rh_value));
            LOG_VERBOSE(BSL_LS_BCM_COMMON,
                        (BSL_META_U(unit,
                                    "\tHG-Trunk DLB Hash value 0x%08x\n"),
                         hash_dlb_value));
 
            BCM_IF_ERROR_RETURN
                (get_td2_hash_hg_trunk(unit, trunk - ti.trunk_fabric_id_min,
                                    hash_value, dst_gport, hash_rh_value, hash_dlb_value,
                                    hgt_rh_enable, hgt_dlb_enable, ethertype));

        }
    } 
    /* vxlan hashing resolution */
    else if (pkt_info->flags & BCM_SWITCH_PKT_INFO_HASH_UDP_SOURCE_PORT) {
        rc = compute_td2_rtag7_vxlan(unit, &hash_res, &hash_value);
        if (rc != BCM_E_NONE) {
            return rc;
        }
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "\tVXlan Hash value 0x%08x\n"),
                     hash_value));
        *dst_intf = hash_value;
    }
    else {
        return BCM_E_PARAM;
    }
    
    return BCM_E_NONE;
}

