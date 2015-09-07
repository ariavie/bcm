/*
 * $Id: 2899acc57b62ec770eba3c18f47cb8635c53f6a7 $
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
 */
#include <sal/types.h>
#ifndef _WCMOD_REGISTER_FIELDS_H_
#define _WCMOD_REGISTER_FIELDS_H_

/* AN advertisement 0 pause fields */

#define AN_IEEE1BLK_AN_ADVERTISEMENT0_PAUSE_pause_MASK   0x0800
#define AN_IEEE1BLK_AN_ADVERTISEMENT0_PAUSE_asm_dir_MASK 0x0400

/* AN advertisement 1 speed fields */

#define AN_IEEE1BLK_AN_ADVERTISEMENT1_TECH_ABILITY_100G_CR10_MASK  0x0400
#define AN_IEEE1BLK_AN_ADVERTISEMENT1_TECH_ABILITY_40G_CR4_MASK    0x0200
#define AN_IEEE1BLK_AN_ADVERTISEMENT1_TECH_ABILITY_40G_KR4_MASK    0x0100
#define AN_IEEE1BLK_AN_ADVERTISEMENT1_TECH_ABILITY_10G_KR_MASK     0x0080
#define AN_IEEE1BLK_AN_ADVERTISEMENT1_TECH_ABILITY_10G_KX4_MASK    0x0040
#define AN_IEEE1BLK_AN_ADVERTISEMENT1_TECH_ABILITY_1G_KX_MASK      0x0020


/* AN advertisement 2 fec fields */

#define AN_IEEE1BLK_AN_ADVERTISEMENT2_FEC_REQUESTED_fec_requested_MASK 0x8000
#define AN_IEEE1BLK_AN_ADVERTISEMENT2_FEC_REQUESTED_fec_ability_MASK 0x8000

#endif
