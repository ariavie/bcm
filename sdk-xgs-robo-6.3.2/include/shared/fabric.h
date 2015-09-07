/*
 * $Id: fabric.h 1.7 Broadcom SDK $
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
 * This file defines common network port parameters.
 *
 * Its contents are not used directly by applications; it is used only
 * by header files of parent APIs which need to define port parameters.
 */

#ifndef _SHR_FABRIC_H
#define _SHR_FABRIC_H

#define _SHR_FABRIC_LINK_STATUS_CRC_ERROR    0x00000001 /* Non-zero CRC rate */
#define _SHR_FABRIC_LINK_STATUS_SIZE_ERROR   0x00000002 /* Non-zero size
                                                          error-count */
#define _SHR_FABRIC_LINK_STATUS_CODE_GROUP_ERROR 0x00000004 /* Non-zero code group
                                                          error-count */
#define _SHR_FABRIC_LINK_STATUS_MISALIGN     0x00000008 /* Link down,
                                                          misalignment error */
#define _SHR_FABRIC_LINK_STATUS_NO_SIG_LOCK  0x00000010 /* Link down, SerDes
                                                          signal lock error */
#define _SHR_FABRIC_LINK_STATUS_NO_SIG_ACCEP 0x00000020 /* Link up, but not
                                                          accepting reachability
                                                          cells */
#define _SHR_FABRIC_LINK_STATUS_ERRORED_TOKENS 0x00000040 /* Low value, indicates
                                                          bad link connectivity
                                                          or link down, based on
                                                          reachability cells */

#define _SHR_FABRIC_LINK_NO_CONNECTIVITY (0xFFFFFFFF) /* FABRIC_LINK_NO_CONNECTIVITY */

/* bcm_fabric_priority_* flags */
#define _SHR_FABRIC_QUEUE_PRIORITY_HIGH_ONLY 0x1        
#define _SHR_FABRIC_QUEUE_PRIORITY_LOW_ONLY  0x2             
#define _SHR_FABRIC_PRIORITY_MULTICAST       0x8  

/*bcm_fabric_multicast_* flgas*/
#define _SHR_FABRIC_MULTICAST_SET_ONLY       0x1
#define _SHR_FABRIC_MULTICAST_COMMIT_ONLY    0x2
#define _SHR_FABRIC_MULTICAST_STATUS_ONLY    0x4

#endif	/* !_SHR_FABRIC_H */

