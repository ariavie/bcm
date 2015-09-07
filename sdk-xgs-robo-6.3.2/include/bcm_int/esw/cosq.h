/*
 * $Id: cosq.h 1.18 Broadcom SDK $
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

#ifndef _BCM_INT_COSQ_H
#define _BCM_INT_COSQ_H

extern int bcm_esw_cosq_deinit(int unit);
extern int _bcm_esw_cosq_config_property_get(int unit);
extern int _bcm_esw_cpu_cosq_mapping_default_set(int unit, int numq);

/* Constant definitions for Katana */

#define BCM_KA_SCHEDULER_CNT                    9
#define BCM_KA_MCAST_QUEUE_GROUP_CNT            4
#define BCM_KA_TOTAL_UCAST_QUEUE_GROUP_CNT      12
#define BCM_KA_REG_UCAST_QUEUE_GROUP_CNT        8
#define BCM_KA_EXT_UCAST_QUEUE_GROUP_MIN        8

typedef enum _bcm_cosq_op_e {
    _BCM_COSQ_OP_SET = 1,
    _BCM_COSQ_OP_ADD,
    _BCM_COSQ_OP_DELETE,
    _BCM_COSQ_OP_CLEAR,
    _BCM_COSQ_OP_GET,
    _BCM_COSQ_OP_COUNT
} _bcm_cosq_op_t;

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_cosq_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_cosq_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

/* The classifier_id used by bcm_cosq_classifier_* APIs
 * contains two fields, a 6-bit type field, followed by
 * a 26-bit value field.
 */
#define _BCM_COSQ_CLASSIFIER_TYPE_NONE      0
#define _BCM_COSQ_CLASSIFIER_TYPE_ENDPOINT  (1 << 0)
#define _BCM_COSQ_CLASSIFIER_TYPE_SERVICE   (1 << 1)
#define _BCM_COSQ_CLASSIFIER_TYPE_FIELD     (1 << 2)
#define _BCM_COSQ_CLASSIFIER_TYPE_MAX       _BCM_COSQ_CLASSIFIER_TYPE_FIELD

#define _BCM_COSQ_CLASSIFIER_TYPE_SHIFT     26
#define _BCM_COSQ_CLASSIFIER_TYPE_MASK      0x3f
#define _BCM_COSQ_CLASSIFIER_ENDPOINT_SHIFT 0
#define _BCM_COSQ_CLASSIFIER_ENDPOINT_MASK  0x3ffffff
#define _BCM_COSQ_CLASSIFIER_SERVICE_SHIFT  0
#define _BCM_COSQ_CLASSIFIER_SERVICE_MASK   0x3ffffff
#define _BCM_COSQ_CLASSIFIER_FIELD_SHIFT    (0)
#define _BCM_COSQ_CLASSIFIER_FIELD_MASK     (0x3ffffff)

#define _BCM_COSQ_CLASSIFIER_IS_SET(_classifier)                     \
        ((((_classifier) >> _BCM_COSQ_CLASSIFIER_TYPE_SHIFT) > 0) && \
         (((_classifier) >> _BCM_COSQ_CLASSIFIER_TYPE_SHIFT) <=      \
          _BCM_COSQ_CLASSIFIER_TYPE_MAX))

#define _BCM_COSQ_CLASSIFIER_IS_ENDPOINT(_classifier)          \
        (((_classifier) >> _BCM_COSQ_CLASSIFIER_TYPE_SHIFT) == \
         _BCM_COSQ_CLASSIFIER_TYPE_ENDPOINT)

#define _BCM_COSQ_CLASSIFIER_ENDPOINT_SET(_classifier, _endpoint)               \
        ((_classifier) = (_BCM_COSQ_CLASSIFIER_TYPE_ENDPOINT <<                 \
                          _BCM_COSQ_CLASSIFIER_TYPE_SHIFT) |                    \
                         (((_endpoint) & _BCM_COSQ_CLASSIFIER_ENDPOINT_MASK) << \
                          _BCM_COSQ_CLASSIFIER_ENDPOINT_SHIFT))

#define _BCM_COSQ_CLASSIFIER_ENDPOINT_GET(_classifier)            \
        (((_classifier) >> _BCM_COSQ_CLASSIFIER_ENDPOINT_SHIFT) & \
         _BCM_COSQ_CLASSIFIER_ENDPOINT_MASK)

#define _BCM_COSQ_CLASSIFIER_IS_SERVICE(_classifier)          \
        (((_classifier) >> _BCM_COSQ_CLASSIFIER_TYPE_SHIFT) == \
         _BCM_COSQ_CLASSIFIER_TYPE_SERVICE)

#define _BCM_COSQ_CLASSIFIER_SERVICE_SET(_classifier, _id)               \
        ((_classifier) = (_BCM_COSQ_CLASSIFIER_TYPE_SERVICE <<                 \
                          _BCM_COSQ_CLASSIFIER_TYPE_SHIFT) |                    \
                         (((_id) & _BCM_COSQ_CLASSIFIER_SERVICE_MASK) << \
                          _BCM_COSQ_CLASSIFIER_SERVICE_SHIFT))

#define _BCM_COSQ_CLASSIFIER_SERVICE_GET(_classifier)            \
        (((_classifier) >> _BCM_COSQ_CLASSIFIER_SERVICE_SHIFT) \
            & _BCM_COSQ_CLASSIFIER_SERVICE_MASK)

#define _BCM_COSQ_CLASSIFIER_IS_FIELD(_classifier)                  \
            (((_classifier) >> _BCM_COSQ_CLASSIFIER_TYPE_SHIFT)     \
                & _BCM_COSQ_CLASSIFIER_TYPE_FIELD)

#define _BCM_COSQ_CLASSIFIER_FIELD_SET(_classifier, _id)                    \
            ((_classifier) = (_BCM_COSQ_CLASSIFIER_TYPE_FIELD               \
                                << _BCM_COSQ_CLASSIFIER_TYPE_SHIFT) |       \
                                (((_id) & _BCM_COSQ_CLASSIFIER_FIELD_MASK)  \
                                << _BCM_COSQ_CLASSIFIER_FIELD_SHIFT))

#define _BCM_COSQ_CLASSIFIER_FIELD_GET(_classifier)                 \
            (((_classifier) >> _BCM_COSQ_CLASSIFIER_FIELD_SHIFT)    \
                & _BCM_COSQ_CLASSIFIER_FIELD_MASK)

#endif  /* !_BCM_INT_COSQ_H */
