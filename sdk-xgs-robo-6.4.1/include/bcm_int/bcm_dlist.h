/*
 * $Id: bcm_dlist.h,v 1.11 Broadcom SDK $
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
 */
#ifdef BCM_DLIST_ENTRY


#ifdef BCM_ROBO_SUPPORT
BCM_DLIST_ENTRY(robo)
#endif

#ifdef BCM_ESW_SUPPORT
BCM_DLIST_ENTRY(esw)
#endif

#ifdef BCM_SBX_SUPPORT
BCM_DLIST_ENTRY(sbx)
#endif

#ifdef BCM_FE2000_SUPPORT
BCM_DLIST_ENTRY(fe2000)
#endif

#ifdef BCM_PETRA_SUPPORT
BCM_DLIST_ENTRY(petra)
#endif

#ifdef BCM_DFE_SUPPORT
BCM_DLIST_ENTRY(dfe)
#endif

#ifdef BCM_RPC_SUPPORT
BCM_DLIST_ENTRY(client) 
#endif

#ifdef BCM_LOOP_SUPPORT
BCM_DLIST_ENTRY(loop)
#endif

#ifdef BCM_TR3_SUPPORT
BCM_DLIST_ENTRY(tr3)
#endif

#ifdef BCM_SHADOW_SUPPORT
BCM_DLIST_ENTRY(shadow)
#endif   
      
#ifdef BCM_TK371X_SUPPORT
BCM_DLIST_ENTRY(tk371x)
#endif

#ifdef BCM_CALADAN3_SUPPORT
BCM_DLIST_ENTRY(caladan3)
#endif

#undef BCM_DLIST_ENTRY


#endif /* BCM_DLIST_ENTRY */
