/*
 * $Id: type27.h,v 1.2 Broadcom SDK $
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
 * File:        soc/shared/dcbformats/type27.h
 * Purpose:     Define dma control block (DCB) format for a type27 DCB
 *              used by the 88230 (sirius)
 *
 *              This file is shared between the SDK and the embedded applications.
 */

#ifndef _SOC_SHARED_DCBFORMATS_TYPE27_H
#define _SOC_SHARED_DCBFORMATS_TYPE27_H

/*
 * DMA Control Block - Type 27
 * Used on 88230 devices
 * 16 words
 */
typedef struct {
        uint32  addr;                   /* T27.0: physical address */
                                        /* T27.1: Control 0 */
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

        uint32  mh0;                    /* T27.2: Module Header word 0 */
        uint32  mh1;                    /* T27.3: Module Header word 1 */
        uint32  mh2;                    /* T27.4: Module Header word 2 */
        uint32  mh3;                    /* T27.5: Module Header word 3 */
        uint32  mh_ext0;                /* T27.6: Module Header Extension word 0 */

#ifdef  LE_HOST
        uint32  reserved;               /* T27.7: Reserved */

                                        /* T27.8 */
        uint32  oi2qb_queue_mc_hi:6,    /* Oi2qb multicast queue ID [17:12] */
                oi2qb_queue_req:18,     /* Oi2qb requeue queue ID [17:0] */
	        :10;                    /* Reserved */

                                        /* T27.9 */
        uint32  sfh_hdr_79_64:6,        /* Sbx Fabric Header [79:64] */
	        class_reso_addr:10,     /* Class resolution address [9:0] */
	        pkt_class:4,            /* Packet Class */
	        oi2qb_queue_mc_lo:12;   /* Oi2qb multicast queue ID [11:0] */

                                        /* T27.10 */	        
        uint32  sfh_hdr_63_32;          /* Sbx Fabric Header [63:32] */

                                        /* T27.11 SBX fabric header */
        uint32  sfh_hdr_31_0;           /* Sbx Fabric Header [31:0] */

                                        /* T27.12: PBE [70:64] */
        uint32  pbe_70_64:7,            /* PBE (egress cell control bus) [70:64] */
                :25;                    /* Reserved */

        uint32  pbe_63_32;              /* T27.13: PBE [63:32] */
        uint32  pbe_31_0;               /* T27.14: PBE [31:0] */

                                        /* T22.15: DMA Status 0 */
        uint32  count:16,               /* Transferred byte count */
                end:1,                  /* End bit (RX) */
                start:1,                /* Start bit (RX) */
                error:1,                /* Cell Error (RX) */
                :12,                    /* Reserved */
                done:1;                 /* Descriptor Done */
#else
        uint32  reserved;               /* T27.7: Reserved */

                                        /* T27.8 */
        uint32  :10,                    /* Reserved */
	        oi2qb_queue_req:18,     /* Oi2qb requeue queue ID [17:0] */
	        oi2qb_queue_mc_hi:6;    /* Oi2qb multicast queue ID [17:12] */

                                        /* T27.9 */
        uint32  oi2qb_queue_mc_lo:12,   /* Oi2qb multicast queue ID [11:0] */
	        pkt_class:4,            /* Packet Class */
		class_reso_addr:10,     /* Class resolution address [9:0] */
	        sfh_hdr_79_64:6;        /* Sbx Fabric Header [79:64] */

                                        /* T27.10 */	        
        uint32  sfh_hdr_63_32;          /* Sbx Fabric Header [63:32] */

                                        /* T27.11 SBX fabric header */
        uint32  sfh_hdr_31_0;           /* Sbx Fabric Header [31:0] */

                                        /* T27.12: PBE [70:64] */
        uint32  :25,                    /* Reserved */
	        pbe_70_64:7;            /* PBE (egress cell control bus) [70:64] */

        uint32  pbe_63_32;              /* T27.13: PBE [63:32] */
        uint32  pbe_31_0;               /* T27.14: PBE [31:0] */

                                        /* T27.15: DMA Status 0 */
        uint32  done:1,                 /* Descriptor Done */
                :12,                    /* Reserved */
                error:1,                /* Cell Error (RX) */
                start:1,                /* Start bit (RX) */
                end:1,                  /* End bit (RX) */
                count:16;               /* Transferred byte count */
#endif
} dcb27_t;
#endif
