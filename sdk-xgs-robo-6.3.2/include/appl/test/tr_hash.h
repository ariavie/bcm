/*
 * $Id: tr_hash.h 1.3 Broadcom SDK $
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
 * TRX Hash Test definitions.
 *
 */

#ifndef __TR_HASH__H
#define __TR_HASH__H

/*
 * TRX Hash Test Data Structure
 */

typedef struct tr_hash_testdata_s {
    int                 unit;
    int                 opt_count;
    int                 opt_verbose;
    int                 opt_reset;
    int                 opt_hash;
    int                 opt_dual_hash;
    int                 opt_base_ovid;
    int                 opt_base_ivid;
    int                 opt_vid_inc;         /* VID increment */
    int                 opt_base_label;
    int                 opt_label_inc;       /* MPLS label increment */
    COMPILER_DOUBLE     tm;
    uint32              save_hash_control;
    int                 hash_count;          /* Hash Alg count */
} tr_hash_testdata_t;

#define TR_VID_MAX            (0xfff)
#define TR_LABEL_MAX          (0xfffff)
#define TR_DEFAULT_HASH       FB_HASH_LSB

/* Test structure, to retain test settings between runs */
typedef struct tr_hash_test_s {
    int                    lw_set_up;  /* TRUE if tr_hash_setup() done */
    tr_hash_testdata_t     lp_hash;    /* hash Parameters */
    tr_hash_testdata_t     lp_ov;      /* overflow Parameters */
    tr_hash_testdata_t     *lp_cur;    /* Current test parameters */
    int                    lw_unit;    /* Unit # */
} tr_hash_test_t;

#endif /*!__TR_HASH__H */
