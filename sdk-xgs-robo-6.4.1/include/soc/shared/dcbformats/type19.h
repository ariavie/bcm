/*
 * $Id: type19.h,v 1.4 Broadcom SDK $
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
 * File:        soc/shared/dcbformats/type19.h
 * Purpose:     Define dma control block (DCB) format for a type19 DCB
 *              used by the 56640 (Triumph3/Firebolt4)
 *
 *              This file is shared between the SDK and the embedded applications.
 */

#ifndef _SOC_SHARED_DCBFORMATS_TYPE19_H
#define _SOC_SHARED_DCBFORMATS_TYPE19_H

/*
 * DMA Control Block - Type 19
 * Used on 5663x devices
 * 16 words
 */
typedef struct {
        uint32  addr;                   /* T19.0: physical address */
                                        /* T19.1: Control 0 */
#ifdef  LE_HOST
        uint32  c_count:16,             /* Requested byte count */
                c_chain:1,              /* Chaining */
                c_sg:1,                 /* Scatter Gather */
                c_reload:1,             /* Reload */
                c_hg:1,                 /* Higig (TX) */
                c_stat:1,               /* update stats (TX) */
                c_pause:1,              /* Pause packet (TX) */
                c_purge:1,              /* Purge packet (TX) */
                :9;                     /* Don't care */
#else
        uint32  :9,                     /* Don't care */
                c_purge:1,              /* Purge packet (TX) */
                c_pause:1,
                c_stat:1,
                c_hg:1,
                c_reload:1,
                c_sg:1,
                c_chain:1,
                c_count:16;
#endif  /* LE_HOST */
        uint32  mh0;                    /* T19.2: Module Header word 0 */
        uint32  mh1;                    /* T19.3: Module Header word 1 */
        uint32  mh2;                    /* T19.4: Module Header word 2 */
        uint32  mh3;                    /* T19.5: Module Header word 3 */
#ifdef  LE_HOST
                                        /* T19.6: RX Status 0 */
        uint32  :3,                     /* Reserved */
                mtp_index:5,            /* MTP index */
                cpu_cos:6,              /* COS queue for CPU packets */
                :2,                     /* Reserved */
                inner_vid:12,           /* Inner VLAN ID */
                inner_cfi:1,            /* Inner Canoncial Format Indicator */
                inner_pri:3;            /* Inner priority */

                                        /* T19.7 */ 
        uint32  reason_hi:16,           /* CPU opcode (high bits) */
                pkt_len:14,             /* Packet length */
                :2;                     /* Reserved */

                                        /* T19.8 */
        uint32  reason;                 /* CPU opcode */

                                        /* T19.9 */
        uint32  dscp:8,                 /* New DSCP */
                chg_tos:1,              /* DSCP has been changed by HW */
                decap_tunnel_type:4,    /* Tunnel type that was decapsulated */
                regen_crc:1,            /* Packet modified and needs new CRC */
                :2,                     /* Reserved */
                outer_vid:12,           /* Outer VLAN ID */
                outer_cfi:1,            /* Outer Canoncial Format Indicator */
                outer_pri:3;            /* Outer priority */

                                        /* T19.10 */
        uint32  timestamp;              /* Timestamp */

                                        /* T19.11 */
        uint32  cos:4,                  /* COS */
                higig_cos:5,            /* Higig COS */
                vlan_cos:5,             /* VLAN COS */
                shaping_cos_sel:2,      /* Shaping COS Select */
                vfi:12,                 /* Internal VFI value */
                :4;                     /* Reserved */

                                        /* T19.12 */
        uint32  srcport:8,              /* Source port number */
                hgi:2,                  /* Higig Interface Format Indicator */
                itag_status:2,          /* Ingress incoming tag status */
                otag_action:2,          /* Ingress Outer tag action */
                itag_action:2,          /* Ingress Inner tag action */
		service_tag:1,          /* SD tag present */
		switch_pkt:1,           /* Switched packet */
                hg_type:1,              /* 0: Higig+, 1: Higig2 */
                src_hg:1,               /* Source is Higig */
                l3routed:1,             /* Any IP routed packet */
                l3only:1,               /* L3 only IPMC packet */
                replicated:1,           /* Replicated copy */
                imirror:1,              /* Ingress Mirroring */
                emirror:1,              /* Egress Mirroring */
                do_not_change_ttl:1,    /* Do not change TTL */
                bpdu:1,                 /* BPDU Packet */
                :5;                     /* Reserved */

                                        /* T19.13 */
        uint32  orig_dstport:6,         /* Original dst port (EP redirection) */
                true_egr_mirror:1,      /* True egress mirrored */
                egr_cpu_copy:1,         /* True egress copy-to-CPU */
                oam_pkt:1,              /* OAM packet */
                eh_tbl_idx:2,           /* Extended Higig table select */
                eh_tag_type:2,          /* Extended Higig tag type */
                eh_tm:1,                /* Extended Higig traffic manager ctrl */
                eh_queue_tag:16,        /* Extended Higig queue tag */
                :2;                     /* Reserved */

                                        /* T19.14 */
        uint32  l3_intf:15,             /* L3 Intf num / Next hop idx */
                :1,                     /* Reserved */
                match_rule:8,           /* Matched FP rule */
                :8;                     /* Reserved */

                                        /* T19.15: DMA Status 0 */
        uint32  count:16,               /* Transferred byte count */
                end:1,                  /* End bit (RX) */
                start:1,                /* Start bit (RX) */
                error:1,                /* Cell Error (RX) */
                dc:12,                  /* Don't Care */
                done:1;                 /* Descriptor Done */
#else
                                        /* T19.6: RX Status 0 */
        uint32  inner_pri:3,            /* Inner priority */
                inner_cfi:1,            /* Inner Canoncial Format Indicator */
                inner_vid:12,           /* Inner VLAN ID */
                :2,                     /* Reserved */
                cpu_cos:6,              /* COS queue for CPU packets */
                mtp_index:5,            /* MTP index */
                :3;                     /* Reserved */

                                        /* T19.7 */
        uint32  :2,                     /* Reserved */
                pkt_len:14,             /* Packet length */
                reason_hi:16;           /* CPU opcode (high bits) */

                                        /* T19.8 */
        uint32  reason;                 /* CPU opcode */

                                        /* T19.9 */
        uint32  outer_pri:3,            /* Outer priority */
                outer_cfi:1,            /* Outer Canoncial Format Indicator */
                outer_vid:12,           /* Outer VLAN ID */
                :2,                     /* Reserved */
                regen_crc:1,            /* Packet modified and needs new CRC */
                decap_tunnel_type:4,    /* Tunnel type that was decapsulated */
                chg_tos:1,              /* DSCP has been changed by HW */
                dscp:8;                 /* New DSCP */

                                        /* T19.10 */
        uint32  timestamp;              /* Timestamp */

                                        /* T19.11 */
        uint32  :4,                     /* Reserved */
                vfi:12,                 /* Internal VFI value */
                shaping_cos_sel:2,      /* Shaping COS Select */
                vlan_cos:5,             /* VLAN COS */
                higig_cos:5,            /* Higig COS */
                cos:4;                  /* COS */

                                        /* T19.12 */
        uint32  :5,                     /* Reserved */
                bpdu:1,                 /* BPDU Packet */
                do_not_change_ttl:1,    /* Do not change TTL */
                emirror:1,              /* Egress Mirroring */
                imirror:1,              /* Ingress Mirroring */
		replicated:1,           /* Replicated copy */
		l3only:1,               /* L3 only IPMC packet */
                l3routed:1,             /* Any IP routed packet */
                src_hg:1,               /* Source is Higig */
                hg_type:1,              /* 0: Higig+, 1: Higig2 */
                switch_pkt:1,           /* Switched packet */
                service_tag:1,          /* SD tag present */
                itag_action:2,          /* Ingress Inner tag action */
                otag_action:2,          /* Ingress Outer tag action */
                itag_status:2,          /* Ingress incoming tag status */
                hgi:2,                  /* Higig Interface Format Indicator */
                srcport:8;              /* Source port number */

                                        /* T19.13 */
        uint32  :2,                     /* Reserved */
                eh_queue_tag:16,        /* Extended Higig queue tag */
                eh_tm:1,                /* Extended Higig traffic manager ctrl */
                eh_tag_type:2,          /* Extended Higig tag type */
                eh_tbl_idx:2,           /* Extended Higig table select */
                oam_pkt:1,              /* OAM packet */
                egr_cpu_copy:1,         /* True egress copy-to-CPU */
                true_egr_mirror:1,      /* True egress mirrored */
                orig_dstport:6;         /* Original dst port (EP redirection) */

                                        /* T19.14 */
        uint32  :8,                     /* Reserved */
                match_rule:8,           /* Matched FP rule */
                :1,                     /* Reserved */
                l3_intf:15;             /* L3 Intf num / Next hop idx */

                                        /* T19.15: DMA Status 0 */
        uint32  done:1,                 /* Descriptor Done */
                dc:12,                  /* Don't Care */
                error:1,                /* Cell Error (RX) */
                start:1,                /* Start bit (RX) */
                end:1,                  /* End bit (RX) */
                count:16;               /* Transferred byte count */
#endif
} dcb19_t;

#endif
