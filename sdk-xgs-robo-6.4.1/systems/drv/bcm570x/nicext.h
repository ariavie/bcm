/****************************************************************************
 * $Id: nicext.h,v 1.3 Broadcom SDK $
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
 * Name:        nicext.h
 *
 * Description: Broadcom Network Interface Card Extension (NICE) is an 
 *              extension to Linux NET device kernel mode drivers. 
 *              NICE is designed to provide additional functionalities, 
 *              such as receive packet intercept. To support Broadcom NICE, 
 *              the network device driver can be modified by adding an 
 *              device ioctl handler and by indicating receiving packets 
 *              to the NICE receive handler. Broadcom NICE will only be 
 *              enabled by a NICE-aware intermediate driver, such as 
 *              Broadcom Advanced Server Program Driver (BASP). When NICE 
 *              is not enabled, the modified network device drivers 
 *              functions exactly as other non-NICE aware drivers.
 *
 * Author:      Frankie Fan
 *
 * Created:     September 17, 2000
 *
 ****************************************************************************/
#ifndef _nicext_h_
#define _nicext_h_

/*
 * ioctl for NICE
 */
#define SIOCNICE                   	SIOCDEVPRIVATE+7

/*
 * SIOCNICE: 
 *
 * The following structure needs to be less than IFNAMSIZ (16 bytes) because
 * we're overloading ifreq.ifr_ifru.
 *
 * If 16 bytes is not enough, we should consider relaxing this because
 * this is no field after ifr_ifru in the ifreq structure. But we may
 * run into future compatiability problem in case of changing struct ifreq.
 */
struct nice_req
{
    __u32 cmd;
    
    union
    {
#ifdef __KERNEL__
        /* cmd = NICE_CMD_SET_RX or NICE_CMD_GET_RX */
        struct
        {
            void (*nrqus1_rx)( struct sk_buff*, void* );
            void* nrqus1_ctx;
        } nrqu_nrqus1;

        /* cmd = NICE_CMD_QUERY_SUPPORT */
        struct
        {
            __u32 nrqus2_magic;
            __u32 nrqus2_support_rx:1;
            __u32 nrqus2_support_vlan:1;
            __u32 nrqus2_support_get_speed:1;
        } nrqu_nrqus2;
#endif

        /* cmd = NICE_CMD_GET_SPEED */
        struct
        {
            unsigned int nrqus3_speed; /* 0 if link is down, */
                                       /* otherwise speed in Mbps */
        } nrqu_nrqus3;

        /* cmd = NICE_CMD_BLINK_LED */
        struct
        {
            unsigned int nrqus4_blink_time; /* blink duration in seconds */
        } nrqu_nrqus4;

    } nrq_nrqu;
};

#define nrq_rx           nrq_nrqu.nrqu_nrqus1.nrqus1_rx
#define nrq_ctx          nrq_nrqu.nrqu_nrqus1.nrqus1_ctx
#define nrq_support_rx   nrq_nrqu.nrqu_nrqus2.nrqus2_support_rx
#define nrq_magic        nrq_nrqu.nrqu_nrqus2.nrqus2_magic
#define nrq_support_vlan nrq_nrqu.nrqu_nrqus2.nrqus2_support_vlan
#define nrq_support_get_speed nrq_nrqu.nrqu_nrqus2.nrqus2_support_get_speed
#define nrq_speed        nrq_nrqu.nrqu_nrqus3.nrqus3_speed
#define nrq_blink_time   nrq_nrqu.nrqu_nrqus4.nrqus4_blink_time

/*
 * magic constants
 */
#define NICE_REQUESTOR_MAGIC            0x4543494E /* NICE in ascii */
#define NICE_DEVICE_MAGIC               0x4E494345 /* ECIN in ascii */

/*
 * command field
 */
#define NICE_CMD_QUERY_SUPPORT          0x00000001
#define NICE_CMD_SET_RX                 0x00000002
#define NICE_CMD_GET_RX                 0x00000003
#define NICE_CMD_GET_SPEED              0x00000004
#define NICE_CMD_BLINK_LED              0x00000005

#endif  /* _nicext_h_ */

