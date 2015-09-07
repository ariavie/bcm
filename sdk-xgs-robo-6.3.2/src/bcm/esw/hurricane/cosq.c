/*
 * $Id: cosq.c 1.35 Broadcom SDK $
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
 * COS Queue Management
 * Purpose: API to set different cosq, priorities, and scheduler registers.
 *
 */

#include <sal/core/libc.h>

#include <soc/defs.h>
#if defined(BCM_HURRICANE_SUPPORT)
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/profile_mem.h>
#include <soc/debug.h>
#include <soc/hurricane.h>

#include <bcm/error.h>
#include <bcm/cosq.h>

#include <bcm_int/esw/cosq.h>
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/hurricane.h>

#include <bcm_int/esw_dispatch.h>

#ifdef BCM_WARM_BOOT_SUPPORT
#include <bcm_int/esw/switch.h>
#endif /* BCM_WARM_BOOT_SUPPORT */


#define BCM_HU_MTU_QUANTA_ID_COUNT 4
#define BCM_HU_COS_WEIGHT_MAX      0x7f



/* COSQ where output of hierarchical COSQs scheduler is attached */

/* MMU cell size in bytes */


#define	MMU_CELL_SIZE		128	/* not overrideable */
#define   MMU_FLOW_PERCENT	90
#define   MMU_FLOW_FANIN		4
#define   MMU_FLOW_PKTMIN		8	/* not overrideable */
#define	MMU_RED_DROP_PERCENT	60
#define	MMU_YELLOW_DROP_PERCENT	80
#define	MMU_STATIC_BYTES	(1536 * 1)
#define	MMU_STATIC_PERCENT	50	/* not used if STATIC_BYTES set */
#define	MMU_RESET_BYTES		(1536 * 2)
#define	MMU_RESET_MIN_PERCENT	50	/* not overrideable */
#define	MMU_OVERCOMMIT		1	/* non-stack dynamic cell overcommit */
#define	MMU_OVERCOMMIT_STACK	2	/* stack dynamic cell overcommit */

#define BCM_HU_CNG_PKT_LIMIT_GRANULARITY       4
#define HU_DRR_KBYTES   (1)
#define HU_DRR_WEIGHT_MAX       0x7f
#define HU_WRR_WEIGHT_MAX       0x7f

/* COS Map profile entries per set */
#define HU_COS_MAP_ENTRIES_PER_SET    16

#define HU_BURST_GRANULARITY   64 /* 64bytes */
#define HU_BW_FIELD_MAX        0x3FFFF



/* Cache of COS_MAP Profile Table */
STATIC soc_profile_mem_t *_hu_cos_map_profile[BCM_MAX_NUM_UNITS] = {NULL};

/* Number of COSQs configured for this device */
STATIC int _hu_num_cosq[SOC_MAX_NUM_DEVICES];


STATIC soc_field_t
_hu_cosfld[] = {
                COS0THDMODEf,
                COS1THDMODEf,
                COS2THDMODEf,
                COS3THDMODEf,
                COS4THDMODEf,
                COS5THDMODEf,
                COS6THDMODEf,
                COS7THDMODEf,
            };

/*      
 *  Convert HW drop probability to percent value
 */     
STATIC int       
_bcm_hu_hw_drop_prob_to_api_val[] = {
    1000,  /* 0  */
    125,   /* 1  */
    63,    /* 2  */
    31,    /* 3  */
    16,    /* 4  */
    8,     /* 5  */
    4,     /* 6  */
    2,     /* 7  */
};

static int MTU_HU_Quanta[BCM_HU_MTU_QUANTA_ID_COUNT] = {
    2 *  HU_DRR_KBYTES,     /* Quanta of 2048 bytes */
    4 *  HU_DRR_KBYTES,     /* Quanta of 4096 bytes */
    8 *  HU_DRR_KBYTES,     /* Quanta of 8192 bytes */
   16 *  HU_DRR_KBYTES      /* Quanta of 16384 bytes */
};

/*Forward Declaration*/
STATIC int
_bcm_hu_cosq_port_sched_get(int unit, soc_reg_t config_reg, 
                            soc_reg_t weight_reg, bcm_port_t port,
                            bcm_cos_queue_t start_cosq,
                            int num_weights, int weights[], int *mode);

STATIC int
_bcm_hu_cosq_config_set(int unit, int numq);

STATIC int     
_bcm_hu_cosq_port_sched_set(int unit, soc_reg_t config_reg, 
                            soc_reg_t weight_reg, bcm_port_t port,
                            bcm_cos_queue_t start_cosq,
                            int num_weights, const int weights[], 
                            int mode);

STATIC int
_bcm_hu_api_val_to_hw_drop_prob(int api_val) {
   int i;
   for (i = 7; i > 0 ; i--) {
      if (api_val <= _bcm_hu_hw_drop_prob_to_api_val[i]) {
          break;
      }
   }
   return i;
}

static int
_bcm_hu_cos_max_weight(const int weight[])
{
    int i, max_weight = 0;

    /* find max weight */
    for (i = 0; i < 8; i++ ) {
        if (max_weight < weight[i]) {
            max_weight = weight[i];
        }
    }
    return max_weight;
}


STATIC int
_bcm_hu_cosq_resolve_mod_port(int unit, bcm_gport_t gport,
                              bcm_module_t *modid, bcm_port_t *port,
                              bcm_trunk_t *trunk_id, int *is_local)
{
    int gport_id;

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, gport, modid,
                                port, trunk_id, &gport_id));
    BCM_IF_ERROR_RETURN(
        _bcm_esw_modid_is_local(unit, *modid, is_local));

    return BCM_E_NONE;
}

/*
 * Calculates the Quanta needed for COS according to its weight
 */
static int
_bcm_hu_cos_drr_weights_to_quanta(const int weight[])
{
    int i, max_weight = 0;

    max_weight = _bcm_hu_cos_max_weight(weight);

    for (i = 0; i < BCM_HU_MTU_QUANTA_ID_COUNT; i++ ) {
        if (max_weight <= MTU_HU_Quanta[i] * BCM_HU_COS_WEIGHT_MAX) {
            return i;   /* Right Quanta was found */
        }
    }
    return -1;  /* In case of too big weight return negative value */
}
/*
 * Convert the number of kbytes (1024 bytes) that can transmtted in one run
 * to weight encoding 
 */
static int
_bcm_hu_mtu_cos_drr_kbytes_to_weight(int kbytes, int MTUQuantaSel)
{
    int cos_weight = 0;
    int weight_found = FALSE, low_weight_boundary, hi_weight_boundary;

    /* Search for right weight */
    low_weight_boundary = 1;
    hi_weight_boundary = BCM_HU_COS_WEIGHT_MAX;

    /* Boundary conditions */
    if (kbytes >= hi_weight_boundary * MTU_HU_Quanta[MTUQuantaSel]) {
        cos_weight = hi_weight_boundary;
        weight_found = TRUE;
    }
    if (kbytes <= low_weight_boundary * MTU_HU_Quanta[MTUQuantaSel]) {
        cos_weight = low_weight_boundary;
        weight_found = TRUE;
    }
    while (!weight_found) {
        cos_weight =  (hi_weight_boundary + low_weight_boundary) / 2;
        if (kbytes <= cos_weight * MTU_HU_Quanta[MTUQuantaSel]) {
           if (kbytes > ((cos_weight - 1) * MTU_HU_Quanta[MTUQuantaSel])) {
               weight_found = TRUE;
           } else {
               hi_weight_boundary = cos_weight;
           }
        } else {
            if (kbytes <= ((cos_weight + 1) * MTU_HU_Quanta[MTUQuantaSel])) {
                cos_weight++;
                weight_found = TRUE;
            } else {
                low_weight_boundary = cos_weight;
            }
        }
    } /* Here weight should be found */
    return cos_weight;
}

/*
 * Convert the encoded weights to number of kbytes that can transmtted
 * in one run.
 */
static int
_bcm_hu_mtu_cos_drr_weight_to_kbytes(int weight, int MTUQuantaSel)
{
    return (weight * MTU_HU_Quanta[MTUQuantaSel]);
}

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_0

/*
 * Function:
 *      bcm_hu_cosq_sync
 * Purpose:
 *      Record COSQ module persistent info for Level 2 Warm Boot.
 * Parameters:
 *      unit - Unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_hu_cosq_sync(int unit)
{
    soc_scache_handle_t scache_handle;
    uint8               *cosq_scache_ptr;
    uint32              num_cosq;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_COSQ, 0);
    BCM_IF_ERROR_RETURN
        (_bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0, &cosq_scache_ptr, 
                                 BCM_WB_DEFAULT_VERSION, NULL));
    
    /* Number COSQ */
    num_cosq = _hu_num_cosq[unit];
    sal_memcpy(cosq_scache_ptr, &num_cosq, sizeof(num_cosq));
        
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_hu_cosq_reinit
 * Purpose:
 *      Recover COSQ software state.
 * Parameters:
 *      unit - Unit number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_hu_cosq_reinit(int unit)
{
    int                 rv;
    soc_scache_handle_t scache_handle;
    uint8               *cosq_scache_ptr;
    uint32              num_cosq;
 	 
    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_COSQ, 0);
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0, &cosq_scache_ptr, 
                                 BCM_WB_DEFAULT_VERSION, NULL);

    if (rv == BCM_E_NOT_FOUND) {
        cosq_scache_ptr = NULL;
    } else if (BCM_FAILURE(rv)) {
        return rv;
    }

    if (cosq_scache_ptr != NULL) {
        /* Number COSQ */
        sal_memcpy(&num_cosq, cosq_scache_ptr, sizeof(num_cosq));
        _hu_num_cosq[unit] = num_cosq;
    } else {
        _hu_num_cosq[unit] = _bcm_esw_cosq_config_property_get(unit);
    }
    
    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/* Function:
 *      bcm_hu_cosq_init
 * Purpose:
 *      Initialize (clear) all COS schedule/mapping state.
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_hu_cosq_init(int unit)
{
     STATIC int _hu_max_cosq = -1;
    int num_cos = 0;
#ifdef BCM_WARM_BOOT_SUPPORT
    int                 rv;
    soc_scache_handle_t scache_handle;
    uint32              cosq_scache_size;
    uint8               *cosq_scache_ptr;
#endif /* BCM_WARM_BOOT_SUPPORT */

    if (_hu_max_cosq < 0) {
        _hu_max_cosq = NUM_COS(unit);
        NUM_COS(unit) = 8;
    }

    if (!SOC_WARM_BOOT(unit)) {    /* Cold Boot */
        BCM_IF_ERROR_RETURN (bcm_hu_cosq_detach(unit, 0));
    }

    num_cos = _bcm_esw_cosq_config_property_get(unit);

#ifdef BCM_WARM_BOOT_SUPPORT
    /* Warm boot level 2 cache size */
    cosq_scache_size = sizeof(uint32); /* Number COSQ */

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_COSQ, 0);
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle,
                                 (0 == SOC_WARM_BOOT(unit)),
                                 cosq_scache_size, &cosq_scache_ptr, 
                                 BCM_WB_DEFAULT_VERSION, NULL);
    if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
        return rv;
    }

    if (SOC_WARM_BOOT(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_hu_cosq_reinit(unit));
        num_cos = _hu_num_cosq[unit];
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    return bcm_hu_cosq_config_set(unit, num_cos);
}





int     
bcm_hu_cosq_port_sched_set(int unit, bcm_pbmp_t pbm,
                              int mode, const int weights[], int delay)
{
    uint32 wrr;
    int                 port;
    uint32              cosarbsel;
    int mbits = 0;
    int                 i, max_weight;
    int                 enc_weights[8];
    int                 MTUQuantaSel = 0;
    
    COMPILER_REFERENCE(delay);
    mbcm_driver[unit]->mbcm_cosq_sched_weight_max_get(unit, mode, &max_weight);
    if((mode != BCM_COSQ_STRICT) && (mode != BCM_COSQ_ROUND_ROBIN) &&
             (max_weight != BCM_COSQ_WEIGHT_UNLIMITED)) {
        for(i = 0; i < NUM_COS(unit); i++) {
            if((weights[i] < 0) || (weights[i] > max_weight)) {
                return BCM_E_PARAM;
            }
        }
    }

    switch (mode) {
    case BCM_COSQ_STRICT:
        mbits = 0;
        break;
    case BCM_COSQ_ROUND_ROBIN:
        mbits = 1;
        break;
    case BCM_COSQ_WEIGHTED_ROUND_ROBIN:
        mbits = 2;
        /*
         * All weight values must fit within 7 bits.
         * If weight is 0, this queue is run in strict mode,
         * others run in WRR mode.
         */

        for(i = 0; i < NUM_COS(unit); i++) {
            enc_weights[i] = weights[i];
        }
        break;
    case BCM_COSQ_DEFICIT_ROUND_ROBIN:
        mbits = 3;
        if (soc_feature(unit, soc_feature_linear_drr_weight)) {
            MTUQuantaSel = _bcm_hu_cos_drr_weights_to_quanta(weights);
            if (MTUQuantaSel < 0) {
                return BCM_E_PARAM;
            }
            for (i = 0; i < NUM_COS(unit); i++) {
                if (weights[i]) {
                    enc_weights[i] =
                        _bcm_hu_mtu_cos_drr_kbytes_to_weight(weights[i],
                                MTUQuantaSel);
                } else {
                    enc_weights[i] = weights[i];
                }
            }
        }/*soc_feature_linear_drr_weight*/
        break;
    case BCM_COSQ_BOUNDED_DELAY:        /* not supported in xgs */
    default:
        return BCM_E_PARAM;
    }

    PBMP_ITER(pbm, port) {
        cosarbsel = 0;
        soc_reg_field_set(unit, XQCOSARBSELr, &cosarbsel, COSARBf, mbits);
        if ((mode == BCM_COSQ_DEFICIT_ROUND_ROBIN) &&
                (soc_feature(unit, soc_feature_linear_drr_weight))) {
            soc_reg_field_set(unit, XQCOSARBSELr, &cosarbsel,
                    MTU_QUANTA_SELECTf, MTUQuantaSel);
        }
        SOC_IF_ERROR_RETURN(WRITE_XQCOSARBSELr(unit, port, cosarbsel));
    }

    if ((mode == BCM_COSQ_WEIGHTED_ROUND_ROBIN) ||
        (mode == BCM_COSQ_DEFICIT_ROUND_ROBIN)) {
        /*
         * Weighted Fair Queueing scheduling among vaild COSs
         */
        PBMP_ITER(pbm, port) {
            if (soc_feature(unit, soc_feature_linear_drr_weight)) {
                for (i = 0; i < NUM_COS(unit); i++) {
                    SOC_IF_ERROR_RETURN(READ_WRRWEIGHT_COSr(unit, port,
                                        i, &wrr));
                    soc_reg_field_set(unit, WRRWEIGHT_COSr, &wrr,
                              WEIGHTf, enc_weights[i]);
                    SOC_IF_ERROR_RETURN(WRITE_WRRWEIGHT_COSr(unit, port,
                                        i, wrr));
                }
            }
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_cosq_port_sched_get
 * Purpose:
 *      Retrieve class-of-service policy and corresponding weights and delay
 * Parameters:
 *      unit     - StrataSwitch unit number.
 *      pbm      - port bitmap
 *      mode     - (output) Scheduling mode, one of BCM_COSQ_xxx
 *      weights  - (output) Weights for each COS queue
 *                          Only for BCM_COSQ_WEIGHTED_ROUND_ROBIN and
 *                          BCM_COSQ_DEFICIT_ROUND_ROBIN mode.
 *                 For DRR Weight is specified in Kbytes
 *      delay    - This parameter is not used
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Actually just returns data for the first port in the bitmap
 */

int
bcm_hu_cosq_port_sched_get(int unit, bcm_pbmp_t pbm,
                           int *mode, int weights[], int *delay)
{
    uint32              cosarbsel, wrr;
    int                 mbits, port,i;
    int                 MTUQuantaSel = -1;	

    mbits = -1;
    PBMP_ITER(pbm, port) {
        SOC_IF_ERROR_RETURN(READ_XQCOSARBSELr(unit, port, &cosarbsel));
        mbits = soc_reg_field_get(unit, XQCOSARBSELr, cosarbsel, COSARBf);
        if(soc_feature(unit, soc_feature_linear_drr_weight)) {
            MTUQuantaSel =
                soc_reg_field_get(unit, XQCOSARBSELr, cosarbsel,
                                  MTU_QUANTA_SELECTf);
        }
        break;
    }
    switch (mbits) {
    case 0:
        *mode = BCM_COSQ_STRICT;
        break;
    case 1:
        *mode = BCM_COSQ_ROUND_ROBIN;
        break;
    case 2:
        *mode = BCM_COSQ_WEIGHTED_ROUND_ROBIN;
        break;
    case 3:
        *mode = BCM_COSQ_DEFICIT_ROUND_ROBIN;
        break;
    default:
        return BCM_E_INTERNAL;
    }
    if ((mbits == 2) || (mbits == 3)) {
        wrr = 0;
        if (soc_feature(unit, soc_feature_linear_drr_weight)) {
            PBMP_ITER(pbm, port) {
                for (i = 0; i < NUM_COS(unit); i++) {
                    SOC_IF_ERROR_RETURN(READ_WRRWEIGHT_COSr(unit, port,
                                                            i, &wrr));
                    weights[i] = soc_reg_field_get(unit, WRRWEIGHT_COSr,
                                                   wrr, WEIGHTf);
                }
            }
        }
        if (mbits == 3) {
            int i;
            for(i = 0; i < NUM_COS(unit); i++) {
                if (soc_feature(unit, soc_feature_linear_drr_weight)) {
                        weights[i] =
                            _bcm_hu_mtu_cos_drr_weight_to_kbytes(weights[i],
                                                             MTUQuantaSel);
                } 
            }
        }
    }
    if (delay) {
        *delay = 0;
    }
    return BCM_E_NONE;
}


int
bcm_hu_cosq_port_bandwidth_set(int unit, bcm_port_t port,
                               bcm_cos_queue_t cosq,
                               uint32 kbits_sec_min,
                               uint32 kbits_sec_max,
                               uint32 kbits_sec_burst,
                               uint32 flags)
{
    uint32 regval, rval = 0;
    uint32 bucket_val = 0;
    uint32  burst_size = 0;
    uint32 refresh_rate, bucketsize, granularity = 3, meter_flags = 0;
    int    refresh_bitsize, bucket_bitsize;

    /*
     * To set the new Bandwidth settings, the procedure adopted is
     * a. reset MAXBUCKETCONFIG, MINBUCKETCONFIG, MAXBUCKET,MINBUCKET
     * b. update MAXBUCKETCONFIG and MINBUCKETCONFIG with new values passed
     * c. if MISCCONFIG.METERING_CLK_EN not set before, enable it.
     */

   /*for hurricane there is only one formula for ITU weight hence no need of selection*/

    /* Disable egress metering for this port */
    BCM_IF_ERROR_RETURN(READ_MAXBUCKETCONFIGr(unit, port, cosq, &regval));
    soc_reg_field_set(unit, MAXBUCKETCONFIGr, &regval, MAX_REFRESHf, 0);
    soc_reg_field_set(unit, MAXBUCKETCONFIGr, &regval, MAX_THD_SELf, 0);
    BCM_IF_ERROR_RETURN(WRITE_MAXBUCKETCONFIGr(unit, port, cosq, regval));

    BCM_IF_ERROR_RETURN(READ_MINBUCKETCONFIGr(unit, port, cosq, &regval));
    soc_reg_field_set(unit, MINBUCKETCONFIGr, &regval, MIN_REFRESHf, 0);
    soc_reg_field_set(unit, MINBUCKETCONFIGr, &regval, MIN_THD_SELf, 0);
    BCM_IF_ERROR_RETURN(WRITE_MINBUCKETCONFIGr(unit, port, cosq, regval));

    /*reset the MAXBUCKET register*/
    soc_reg_field_set(unit, MAXBUCKETr, &bucket_val, MAX_BUCKETf, 0);
    soc_reg_field_set(unit, MAXBUCKETr, &bucket_val, IN_PROFILE_FLAGf, 0);
    BCM_IF_ERROR_RETURN(WRITE_MAXBUCKETr(unit, port, cosq, bucket_val));

    /*reset the MINBUCKET register value*/
    soc_reg_field_set(unit, MINBUCKETr, &bucket_val, MIN_BUCKETf, 0);
    soc_reg_field_set(unit, MINBUCKETr, &bucket_val, IN_PROFILE_FLAGf, 0);
    BCM_IF_ERROR_RETURN(WRITE_MINBUCKETr(unit, port, cosq, bucket_val));

    refresh_bitsize =
        soc_reg_field_length(unit, MINBUCKETCONFIGr, MIN_REFRESHf);
    bucket_bitsize =
        soc_reg_field_length(unit, MINBUCKETCONFIGr, MIN_THD_SELf);

    BCM_IF_ERROR_RETURN
        (_bcm_xgs_kbits_to_bucket_encoding(kbits_sec_min, kbits_sec_min,
                          meter_flags, refresh_bitsize, bucket_bitsize,
                          &refresh_rate, &bucketsize, &granularity));

 
	regval = 0;
    soc_reg_field_set(unit, MINBUCKETCONFIGr, &regval,
                          MIN_REFRESHf, refresh_rate);
    soc_reg_field_set(unit, MINBUCKETCONFIGr, &regval,
		                  MIN_THD_SELf, bucketsize);
    BCM_IF_ERROR_RETURN
        (WRITE_MINBUCKETCONFIGr(unit, port, cosq, regval));

    refresh_bitsize =
        soc_reg_field_length(unit, MAXBUCKETCONFIGr, MAX_REFRESHf);
    bucket_bitsize =
        soc_reg_field_length(unit, MAXBUCKETCONFIGr, MAX_THD_SELf);

    BCM_IF_ERROR_RETURN
        (_bcm_xgs_kbits_to_bucket_encoding(kbits_sec_max, kbits_sec_burst,
                          meter_flags, refresh_bitsize, bucket_bitsize,
                          &refresh_rate, &bucketsize, &granularity));

	regval = 0;
    soc_reg_field_set(unit, MAXBUCKETCONFIGr, &regval,
                          MAX_REFRESHf, refresh_rate);
    if (flags & BCM_COSQ_BW_EAV_MODE) {
        burst_size = (((kbits_sec_burst * 1000) / 8) +
                          (HU_BURST_GRANULARITY - 1)) / HU_BURST_GRANULARITY;
            if (burst_size > HU_BW_FIELD_MAX) {
                burst_size = HU_BW_FIELD_MAX;
            }

        soc_reg_field_set(unit, MAXBUCKETCONFIGr, &regval, EAV_MODEf, 1);

        SOC_IF_ERROR_RETURN(soc_reg32_get(unit, EAVBUCKETCONFIG_EXTr, port, 
                                                                cosq, &rval));
        soc_reg_field_set(unit, EAVBUCKETCONFIG_EXTr, &rval, EAV_THD_SEL_6LSBf,
                                                            burst_size & 0x3f);
        SOC_IF_ERROR_RETURN(soc_reg32_set(unit, EAVBUCKETCONFIG_EXTr, port, 
                                                                  cosq, rval));
        soc_reg_field_set(unit, MAXBUCKETCONFIGr, &regval, MAX_THD_SELf,
                                                   (burst_size >> 6 & 0xFFF)); 
    } else {
        soc_reg_field_set(unit, MAXBUCKETCONFIGr, &regval, MAX_THD_SELf,
                          bucketsize); 
    }
    BCM_IF_ERROR_RETURN(WRITE_MAXBUCKETCONFIGr(unit, port, cosq, regval));

    /* MISCCONFIG.METERING_CLK_EN is set by chip init */

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_cosq_gport_bandwidth_get
 * Purpose:
 *
 * Parameters:
 *      unit - (IN) Unit number.
 *      gport - (IN) GPORT ID.
 *      cosq - (IN) COS queue to configure, -1 for all COS queues.
 *      kbits_sec_min - (OUT) minimum bandwidth, kbits/sec.
 *      kbits_sec_max - (OUT) maximum bandwidth, kbits/sec.
 *      flags - (OUT) BCM_COSQ_BW_*
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_hu_cosq_gport_bandwidth_get(int unit, bcm_gport_t gport,
                                bcm_cos_queue_t cosq, uint32 *kbits_sec_min,
                                uint32 *kbits_sec_max, uint32 *flags)
{
    bcm_module_t modid;
    bcm_port_t local_port;
    bcm_trunk_t trunk_id;
    int is_local;
    uint32 kbits_max_burst; /* Placeholder */

    *kbits_sec_min = *kbits_sec_max = *flags = 0;

    BCM_IF_ERROR_RETURN
        (_bcm_hu_cosq_resolve_mod_port(unit, gport, &modid,
                                       &local_port, &trunk_id, &is_local));
    
    if (is_local) {
		
        if (cosq >= _hu_num_cosq[unit]) {
            return BCM_E_PARAM;
        } else if (cosq < 0) {
            cosq = 0;
        }
       
        BCM_IF_ERROR_RETURN
            (bcm_hu_cosq_port_bandwidth_get(unit, local_port, cosq,
                                            kbits_sec_min, kbits_sec_max,
                                           &kbits_max_burst, flags));
    } else {
        return BCM_E_PORT;
    }
    return BCM_E_NONE;
}

/*  
 * Function:
 *      bcm_hu_cosq_gport_bandwidth_set
 * Purpose:
 *       
 * Parameters:
 *      unit - (IN) Unit number.
 *      gport - (IN) GPORT ID.
 *      cosq - (IN) COS queue to configure, -1 for all COS queues.
 *      kbits_sec_min - (IN) minimum bandwidth, kbits/sec.
 *      kbits_sec_max - (IN) maximum bandwidth, kbits/sec.
 *      flags - (IN) BCM_COSQ_BW_*
 * Returns:
 *      BCM_E_XXX 
 */ 
int
bcm_hu_cosq_gport_bandwidth_set(int unit, bcm_gport_t gport,
                                bcm_cos_queue_t cosq, uint32 kbits_sec_min,
                                uint32 kbits_sec_max, uint32 flags)
{
    bcm_module_t modid;
    bcm_port_t local_port;
    bcm_trunk_t trunk_id;
    int i, is_local;
    bcm_cos_queue_t cosq_start = 0, cosq_end = 0;

    BCM_IF_ERROR_RETURN
        (_bcm_hu_cosq_resolve_mod_port(unit, gport, &modid,
                                       &local_port, &trunk_id, &is_local));

    if (is_local) {

        if (cosq >= _hu_num_cosq[unit]) {
            return BCM_E_PARAM;
        } else if (cosq < 0) {
            cosq_start = 0;
            cosq_end = 7;
        } else {
                cosq_start = cosq_end = cosq;
            }
        for (i = cosq_start; i <= cosq_end; i++) {
            BCM_IF_ERROR_RETURN
                (bcm_hu_cosq_port_bandwidth_set(unit, local_port, i,
                                                kbits_sec_min,
                                                kbits_sec_max,
                                                kbits_sec_max,
                                                flags));
        }
    } else {
        return BCM_E_PORT;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_cosq_gport_sched_get
 * Purpose:
 *
 * Parameters:
 *      unit - (IN) Unit number.
 *      gport - (IN) GPORT ID.
 *      cosq - (IN) COS queue
 *      mode - (OUT) Scheduling mode, one of BCM_COSQ_xxx
 *      weight - (OUT) Weight for the specified COS queue(s)
 *               Unused if mode is BCM_COSQ_STRICT.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_hu_cosq_gport_sched_get(int unit, bcm_gport_t gport, 
                            bcm_cos_queue_t cosq, int *mode, int *weight)
{   
    bcm_module_t modid;
    bcm_port_t local_port;
    bcm_trunk_t trunk_id;
    int is_local;
    soc_reg_t config_reg = XQCOSARBSELr;
    soc_reg_t weight_reg = WRRWEIGHT_COSr;

    *mode = *weight = 0;

    BCM_IF_ERROR_RETURN
        (_bcm_hu_cosq_resolve_mod_port(unit, gport, &modid,
                                       &local_port, &trunk_id, &is_local));
    if (is_local) {

        if (cosq >= _hu_num_cosq[unit]) {
            return BCM_E_PARAM;
        } else if (cosq < 0) {
            cosq = 0;
        }
        BCM_IF_ERROR_RETURN
            (_bcm_hu_cosq_port_sched_get(unit, config_reg, weight_reg, 
                                         local_port, cosq, 1,
                                         weight, mode));
    } else {
        return BCM_E_PORT;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_hu_cosq_gport_sched_set
 * Purpose:
 *
 * Parameters:
 *      unit - (IN) Unit number.
 *      gport - (IN) GPORT ID.
 *      cosq - (IN) COS queue to configure, -1 for all COS queues.
 *      mode - (IN) Scheduling mode, one of BCM_COSQ_xxx
 *      weight - (IN) Weight for the specified COS queue(s)
 *               Unused if mode is BCM_COSQ_STRICT.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_hu_cosq_gport_sched_set(int unit, bcm_gport_t gport,
                            bcm_cos_queue_t cosq, int mode, int weight)
{
    bcm_module_t modid;
    bcm_port_t local_port;
    bcm_trunk_t trunk_id;
    int i, is_local;
    bcm_cos_queue_t cosq_start = 0;
    int num_weights = 1, weights[8];
    soc_reg_t config_reg = XQCOSARBSELr;
    soc_reg_t weight_reg = WRRWEIGHT_COSr;

    sal_memset(weights, 0, sizeof(weights));
    BCM_IF_ERROR_RETURN
        (_bcm_hu_cosq_resolve_mod_port(unit, gport, &modid,
                                       &local_port, &trunk_id, &is_local));

    if (is_local) {
        /*if (!_hu_num_port_cosq[unit]) {
            return BCM_E_INIT;
        }*/

            if (cosq >= _hu_num_cosq[unit]) {
                return BCM_E_PARAM;
            } else if (cosq < 0) {
                cosq_start = 0;
                num_weights = 8;
                for (i = 0; i < num_weights; i++) {
                    if (i < _hu_num_cosq[unit]) {
                        weights[i] = weight;
                    } else {
                        weights[i] = 0;
                    }
                }
            } else {
                cosq_start = cosq;
                num_weights = 1;
                weights[0] = weight;
            }


        BCM_IF_ERROR_RETURN
            (_bcm_hu_cosq_port_sched_set(unit, config_reg, weight_reg, 
                                         local_port, cosq_start, num_weights,
                                         weights, mode));
    } else {
        return BCM_E_PORT;
    }
    return BCM_E_NONE;
}


int
bcm_hu_cosq_port_bandwidth_get(int unit, bcm_port_t port,
                               bcm_cos_queue_t cosq,
                               uint32 *kbits_sec_min,
                               uint32 *kbits_sec_max,
                               uint32 *kbits_sec_burst,
                               uint32 *flags)
{
    uint32 regval;
    uint32 refresh_rate = 0, bucketsize = 0,
        granularity = 3, meter_flags = 0;
    uint32 kbits_min_burst; /* Placeholder, since burst is unused for min */
    uint32 rval = 0;
    uint32 bucket_lo = 0;
    uint32 eav_mode = 0; 
    if (!kbits_sec_min || !kbits_sec_max || !kbits_sec_burst || !flags) {
        return (BCM_E_PARAM);
    }

    *flags = 0;

    BCM_IF_ERROR_RETURN(READ_MAXBUCKETCONFIGr(unit, port, cosq, &regval));
    refresh_rate = soc_reg_field_get(unit, MAXBUCKETCONFIGr,
                                             regval, MAX_REFRESHf);
    bucketsize = soc_reg_field_get(unit, MAXBUCKETCONFIGr,
                                             regval, MAX_THD_SELf);
    eav_mode = soc_reg_field_get(unit, MAXBUCKETCONFIGr,
                                             regval, EAV_MODEf);
    BCM_IF_ERROR_RETURN
        (_bcm_xgs_bucket_encoding_to_kbits(refresh_rate, bucketsize,
                                           granularity, meter_flags,
                                           kbits_sec_max, kbits_sec_burst));
    if (eav_mode == 1) {
        *flags |= BCM_COSQ_BW_EAV_MODE;
        SOC_IF_ERROR_RETURN(soc_reg32_get(unit, EAVBUCKETCONFIG_EXTr, 
                                                  port, cosq, &rval));
        bucket_lo = soc_reg_field_get(unit, EAVBUCKETCONFIG_EXTr, 
                                                  rval, EAV_THD_SEL_6LSBf);
        *kbits_sec_burst = (((bucketsize << 6) | bucket_lo) * 
                                     HU_BURST_GRANULARITY * 8) / 1000;
    }

    BCM_IF_ERROR_RETURN(READ_MINBUCKETCONFIGr(unit, port, cosq, &regval));

    refresh_rate = soc_reg_field_get(unit, MINBUCKETCONFIGr,
                                             regval, MIN_REFRESHf);

    BCM_IF_ERROR_RETURN
        (_bcm_xgs_bucket_encoding_to_kbits(refresh_rate, 0,
                                           granularity, meter_flags,
                                           kbits_sec_min, &kbits_min_burst));


    return BCM_E_NONE;
}


STATIC int
_bcm_hu_cosq_port_sched_get(int unit, soc_reg_t config_reg, 
                            soc_reg_t weight_reg, bcm_port_t port,
                            bcm_cos_queue_t start_cosq,
                            int num_weights, int weights[], int *mode)
{
    uint32 addr, escfg, wrr;
    int mbits, i, cosq;
    int MTUQuantaSel = -1;

    addr = soc_reg_addr(unit, config_reg, port, 0);
    SOC_IF_ERROR_RETURN
        (soc_reg32_read(unit, addr, &escfg));
    mbits = soc_reg_field_get(unit, config_reg, escfg, COSARBf);
    MTUQuantaSel = soc_reg_field_get(unit, config_reg, escfg,
                                     MTU_QUANTA_SELECTf);
    
    switch (mbits) {
    case 0:
        *mode = BCM_COSQ_STRICT;
        break;
    case 1:
        *mode = BCM_COSQ_ROUND_ROBIN;
        break;
    case 2:
        *mode = BCM_COSQ_WEIGHTED_ROUND_ROBIN;
        break;
    case 3:
        *mode = BCM_COSQ_DEFICIT_ROUND_ROBIN;
        break;
    default:
        return BCM_E_INTERNAL;
    }

    if ((mbits == 2) || (mbits == 3)) {
        wrr = 0;
        for(cosq = start_cosq, i = 0; i < num_weights; cosq++, i++) {
            SOC_IF_ERROR_RETURN(READ_WRRWEIGHT_COSr(unit, port,
                        i, &wrr));
            weights[i] = soc_reg_field_get(unit, weight_reg, wrr,
                                           WEIGHTf);
        }
    
        if (mbits == 3) {
            int i;
            for(i = 0; i < num_weights; i++) {
                weights[i] = _bcm_hu_mtu_cos_drr_weight_to_kbytes(weights[i],MTUQuantaSel);
            }
        }
    }   
    return BCM_E_NONE;
}

/*
 * Convert the number of kbytes (1024 bytes) that can transmtted in one run
 * to weight encoding for 
 */
static int
_bcm_hu_cos_drr_kbytes_to_weight(int kbytes, int MTUQuantaSel)
{
    int cos_weight = 0;
    int weight_found = FALSE, low_weight_boundary, hi_weight_boundary;

    /* Search for right weight */
    low_weight_boundary = 1;
    hi_weight_boundary = BCM_HU_COS_WEIGHT_MAX;

    /* Boundary conditions */
    if (kbytes >= hi_weight_boundary * MTU_HU_Quanta[MTUQuantaSel]) {
        cos_weight = hi_weight_boundary;
        weight_found = TRUE;
    }
    if (kbytes <= low_weight_boundary * MTU_HU_Quanta[MTUQuantaSel]) {
        cos_weight = low_weight_boundary;
        weight_found = TRUE;
    }

    while (!weight_found) {
        cos_weight =  (hi_weight_boundary + low_weight_boundary) / 2;
        if (kbytes <= cos_weight * MTU_HU_Quanta[MTUQuantaSel]) {
           if (kbytes > ((cos_weight - 1) * MTU_HU_Quanta[MTUQuantaSel])) {
               weight_found = TRUE;
           } else {
               hi_weight_boundary = cos_weight;
           }
        } else {
            if (kbytes <= ((cos_weight + 1) * MTU_HU_Quanta[MTUQuantaSel])) {
                cos_weight++;
                weight_found = TRUE;
            } else {
                low_weight_boundary = cos_weight;
            }
        }
    } /* Here weight should be found */

    return cos_weight;
}


STATIC int     
_bcm_hu_cosq_port_sched_set(int unit, soc_reg_t config_reg, 
                            soc_reg_t weight_reg, bcm_port_t port,
                            bcm_cos_queue_t start_cosq,
                            int num_weights, const int weights[], 
                            int mode)
{
    int t, i, cosq;
    uint32 wrr;
    int mbits = 0;
    int enc_weights[8];
    int MTUQuantaSel = 0;	
    
    switch (mode) {
    case BCM_COSQ_STRICT:
        mbits = 0;
        break;
    case BCM_COSQ_ROUND_ROBIN:
        mbits = 1;
        break;
    case BCM_COSQ_WEIGHTED_ROUND_ROBIN:
        mbits = 2;
        /*
         * All weight values must fit within 7 bits.
         * If weight is 0, this queue is run in strict mode,
         * others run in WRR mode.
         */
        t = 0;
        for(i = 0; i < num_weights; i++) {
            t |= weights[i];
        }
        if ((t & ~0x7f) != 0) {
            return BCM_E_PARAM;
        }
        for(i = 0; i < num_weights; i++) {
            enc_weights[i] = weights[i];
        }
        break;
    case BCM_COSQ_DEFICIT_ROUND_ROBIN:
        mbits = 3;
	MTUQuantaSel = _bcm_hu_cos_drr_weights_to_quanta(weights);
	if (MTUQuantaSel < 0) {
		return BCM_E_PARAM;
	}

        for(i = 0; i < num_weights; i++) {
            if (weights[i]) {
                enc_weights[i] =
                    _bcm_hu_cos_drr_kbytes_to_weight(weights[i],
                            MTUQuantaSel);
            } else {
                enc_weights[i] = weights[i];
            }

        }
        break;
    case BCM_COSQ_BOUNDED_DELAY:        /* not supported in xgs */
    default:
        return BCM_E_PARAM;
    }

    /* Program the scheduling mode */
    SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, config_reg, port, 
                           COSARBf, mbits));

    if ((mode == BCM_COSQ_WEIGHTED_ROUND_ROBIN) ||
        (mode == BCM_COSQ_DEFICIT_ROUND_ROBIN)) {
        /*
         * Weighted Fair Queueing scheduling among vaild COSs
         */
        for(cosq = start_cosq, i = 0; i < num_weights; cosq++, i++) {
            SOC_IF_ERROR_RETURN(READ_WRRWEIGHT_COSr(unit, port,
                        i, &wrr));
            /*    coverity[uninit_use_in_call : FALSE]    */
            soc_reg_field_set(unit, WRRWEIGHT_COSr, &wrr,
                    WEIGHTf, enc_weights[i]);
            SOC_IF_ERROR_RETURN(WRITE_WRRWEIGHT_COSr(unit, port,
                        i, wrr));

        }
    }
    return BCM_E_NONE;
}




/*
 * Function:
 *      bcm_cosq_detach
 * Purpose:
 *      Discard all COS schedule/mapping state.
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_hu_cosq_detach(int unit, int software_state_only)
{
    bcm_port_t port;
    bcm_pbmp_t port_all;	
    int weights[8];
    int cosq;


    for (cosq = 0; cosq < 8; cosq++) {
        weights[cosq] = 0;
    }

    if (!software_state_only)
    {
        PBMP_ALL_ITER(unit, port) {
            if (IS_CPU_PORT(unit, port)) {
                continue;
            }

            {
                /* Clear bandwidth settings on port */
                for (cosq = 0; cosq < 8; cosq++) {
                     BCM_IF_ERROR_RETURN
                        (bcm_hu_cosq_port_bandwidth_set(unit, port, cosq, 0, 0, 0, 0));
                }
            }
        }
            /* Clear scheduling settings on port */
            cosq = 8;
	 port_all = PBMP_ALL(unit);	
            BCM_IF_ERROR_RETURN
            (bcm_hu_cosq_port_sched_set(unit, port_all, BCM_COSQ_WEIGHTED_ROUND_ROBIN,
                                         weights, 0));
            /* Clear discard settings on port */
     


    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_hu_cosq_config_set
 * Purpose:
 *      Set the number of COS queues
 * Parameters:
 *      unit - Unit number.
 *      numq - number of COS queues (1-8).
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_hu_cosq_config_set(int unit, int numq)
{
    int cos, prio, ratio, remain;
    uint32 index;
    soc_mem_t mem = PORT_COS_MAPm; 
    int entry_words = sizeof(port_cos_map_entry_t) / sizeof(uint32);
    bcm_pbmp_t ports;
    bcm_port_t port;
    port_cos_map_entry_t cos_map_array[HU_COS_MAP_ENTRIES_PER_SET];
    void *entries[HU_COS_MAP_ENTRIES_PER_SET];

    /* Validate cosq num */
    if (numq < 1) {
        return BCM_E_PARAM;
    }

    if (!SOC_WARM_BOOT(unit)) {
        SOC_IF_ERROR_RETURN(_bcm_hu_cosq_config_set(unit, numq));
    }

    /* Map the eight 802.1 priority levels to the active cosqs */
    if (numq > 8) {
        numq = 8;
    }
    
    sal_memset(cos_map_array, 0,
               HU_COS_MAP_ENTRIES_PER_SET * sizeof(port_cos_map_entry_t));

    if (_hu_cos_map_profile[unit] == NULL) {
        _hu_cos_map_profile[unit] = sal_alloc(sizeof(soc_profile_mem_t),
                                          "COS_MAP Profile Mem");
        if (_hu_cos_map_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(_hu_cos_map_profile[unit]);
    }

    /* Create profile table cache (or re-init if it already exists) */
    SOC_IF_ERROR_RETURN(soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                               _hu_cos_map_profile[unit]));

    if (SOC_WARM_BOOT(unit)) {    /* Warm Boot */
        int     i;
        uint32  val;

        /* Gather reference count for cosq map profile table */
        BCM_PBMP_ASSIGN(ports, PBMP_ALL(unit));
        PBMP_ITER(ports, port) {
            SOC_IF_ERROR_RETURN(READ_COS_MAP_SELr(unit, port, &val));
            index = soc_reg_field_get(unit, COS_MAP_SELr, val, SELECTf);
            index *= HU_COS_MAP_ENTRIES_PER_SET;
            for (i = 0; i < HU_COS_MAP_ENTRIES_PER_SET; i++) {
                SOC_PROFILE_MEM_REFERENCE(unit, _hu_cos_map_profile[unit],
                                          index + i, 1);
                SOC_PROFILE_MEM_ENTRIES_PER_SET(unit,
                                                _hu_cos_map_profile[unit],
                                                index + i,
                                                HU_COS_MAP_ENTRIES_PER_SET);
            }
#ifndef BCM_COSQ_HIGIG_MAP_DISABLE
            if (IS_HG_PORT(unit, port) || port == CMIC_PORT(unit)) {
                SOC_IF_ERROR_RETURN(READ_ICOS_MAP_SELr(unit, port, &val));
                index = soc_reg_field_get(unit, ICOS_MAP_SELr, val, SELECTf);
                index *= HU_COS_MAP_ENTRIES_PER_SET;
                for (i = 0; i < HU_COS_MAP_ENTRIES_PER_SET; i++) {
                    SOC_PROFILE_MEM_REFERENCE(unit, _hu_cos_map_profile[unit],
                                              index + i, 1);
                    SOC_PROFILE_MEM_ENTRIES_PER_SET(unit,
                                                    _hu_cos_map_profile[unit],
                                                    index + i,
                                                    HU_COS_MAP_ENTRIES_PER_SET);
                }
            }
#endif /* BCM_COSQ_HIGIG_MAP_DISABLE */
        }

        _hu_num_cosq[unit] = numq;

        return BCM_E_NONE;
    }

    /* Cold Boot */
    ratio = 8 / numq;
    remain = 8 % numq;
    cos = 0;
    for (prio = 0; prio < 8; prio++) {
        soc_mem_field32_set(unit, PORT_COS_MAPm,
                            &cos_map_array[prio], COSf, cos);
        if (soc_mem_field_valid(unit,PORT_COS_MAPm,HG_COSf)) {
            soc_mem_field32_set(unit, PORT_COS_MAPm,
                                &cos_map_array[prio], HG_COSf, 0);
        }

        if ((prio + 1) == (((cos + 1) * ratio) +
                           ((remain < (numq - cos)) ? 0 :
                            (remain - (numq - cos) + 1)))) {
            cos++;
        }
        entries[prio] = (void *) &cos_map_array[prio];
    }

    /* Map remaining internal priority levels to highest priority cosq */
    cos = numq - 1;
    for (prio = 8; prio < 16; prio++) {
        soc_mem_field32_set(unit, PORT_COS_MAPm,
                            &cos_map_array[prio], COSf, cos);
        if (soc_mem_field_valid(unit,PORT_COS_MAPm,HG_COSf)) {
            soc_mem_field32_set(unit, PORT_COS_MAPm,
                            &cos_map_array[prio], HG_COSf, 0);
        }
        entries[prio] = (void *) &cos_map_array[prio];
    }

    /* Map internal priority levels to CPU CoS queues */
    BCM_IF_ERROR_RETURN(_bcm_esw_cpu_cosq_mapping_default_set(unit, numq));

    /* Add a profile mem entry for each port */
    BCM_PBMP_ASSIGN(ports, PBMP_ALL(unit));
    PBMP_ITER(ports, port) {
        SOC_IF_ERROR_RETURN
            (soc_profile_mem_add(unit, _hu_cos_map_profile[unit],
                                 (void *) &entries,
                                 HU_COS_MAP_ENTRIES_PER_SET, &index));
        SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, COS_MAP_SELr, port, 
                               SELECTf, (index / HU_COS_MAP_ENTRIES_PER_SET)));
#ifndef BCM_COSQ_HIGIG_MAP_DISABLE
        if (IS_HG_PORT(unit, port) || port == CMIC_PORT(unit)) {
            SOC_IF_ERROR_RETURN
                (soc_profile_mem_add(unit, _hu_cos_map_profile[unit],
                                     (void *) &entries,
                                     HU_COS_MAP_ENTRIES_PER_SET, &index));
            SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, ICOS_MAP_SELr, port, 
                                   SELECTf, (index / HU_COS_MAP_ENTRIES_PER_SET)));
        }
#endif
    }

#ifdef BCM_COSQ_HIGIG_MAP_DISABLE
    /* identity mapping for higig ports */

    /* map prio0->cos0, prio1->cos1, ... , prio7->cos7 */
    for (prio = 0; prio < 8; prio++) {
        soc_mem_field32_set(unit, PORT_COS_MAPm,
                            &cos_map_array[prio], COSf, prio);
        if (soc_mem_field_valid(unit,PORT_COS_MAPm,HG_COSf)) {
            soc_mem_field32_set(unit, PORT_COS_MAPm,
                            &cos_map_array[prio], HG_COSf, 0);
        }
    }
    /* Map remaining internal priority levels to highest priority cosq */
    for (prio = 8; prio < 16; prio++) {
        soc_mem_field32_set(unit, PORT_COS_MAPm,
                            &cos_map_array[prio], COSf, 7);
        if (soc_mem_field_valid(unit,PORT_COS_MAPm,HG_COSf)) {
            soc_mem_field32_set(unit, PORT_COS_MAPm,
                            &cos_map_array[prio], HG_COSf, 0);
        }
    }
    PBMP_ITER(ports, port) {
        if (IS_HG_PORT(unit, port) || port == CMIC_PORT(unit)) {
            SOC_IF_ERROR_RETURN
                (soc_profile_mem_add(unit, _hu_cos_map_profile[unit],
                                     (void *) &entries,
                                     HU_COS_MAP_ENTRIES_PER_SET, &index));
            soc_reg_field32_modify(unit, ICOS_MAP_SELr, port, 
                                   SELECTf, (index / HU_COS_MAP_ENTRIES_PER_SET));
        }
    }
#endif

    _hu_num_cosq[unit] = numq;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_cosq_config_get
 * Purpose:
 *      Get the number of cos queues
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      numq - (Output) number of cosq
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_hu_cosq_config_get(int unit, int *numq)
{
    if (_hu_num_cosq[unit] == 0) {
        return BCM_E_INIT;
    }

    if (numq != NULL) {
        *numq = _hu_num_cosq[unit];
    }

    return BCM_E_NONE;
}

int
bcm_hu_cosq_discard_set(int unit, uint32 flags)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hu_cosq_discard_get(int unit, uint32* flags)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hu_cosq_gport_discard_set(int unit, bcm_gport_t port, bcm_cos_queue_t cosq,
                              bcm_cosq_gport_discard_t *discard)
{
    return BCM_E_UNAVAIL;
}

int
bcm_hu_cosq_gport_discard_get(int unit, bcm_gport_t port, bcm_cos_queue_t cosq,
                              bcm_cosq_gport_discard_t *discard)
{
    return BCM_E_UNAVAIL;
}                              


int
bcm_hu_cosq_discard_port_set(int unit, bcm_port_t port,
                             bcm_cos_queue_t cosq,
                             uint32 color,
                             int drop_start,
                             int drop_slope,
                             int average_time)
{
    bcm_port_t local_port;
    bcm_pbmp_t pbmp;
    int i, cosq_start, num_cosq;
    uint32 min_thresh, n;
    uint32 rval, encoding;
    soc_reg_t reg;
    soc_field_t fld;

    if (drop_start < 0 || drop_start > 100 || drop_slope > 0) {
        return BCM_E_PARAM;
    }

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &local_port));
        BCM_PBMP_PORT_SET(pbmp, local_port);
    } else {
        if (port == -1) {
            BCM_PBMP_ASSIGN(pbmp, PBMP_ALL(unit));
        } else if (!SOC_PORT_VALID(unit, port)) {
            return BCM_E_PORT;
        } else {
            BCM_PBMP_PORT_SET(pbmp, port);
        }
    }

    if (cosq < -1 || cosq >= _hu_num_cosq[unit]) {
        return BCM_E_PARAM;
    } else if (cosq == -1) {
        cosq_start = 0;
        num_cosq = _hu_num_cosq[unit];
    } else {
        cosq_start = cosq;
        num_cosq = 1;
    }

    BCM_PBMP_ITER(pbmp, local_port) {
        for (i = cosq_start; i < (cosq_start + num_cosq); i++) { 
            /* Get the start point as a function of the HOL limit */
            SOC_IF_ERROR_RETURN
                (READ_HOLCOSPKTSETLIMITr(unit, local_port, i, &rval));
            n = soc_reg_field_get(unit, HOLCOSPKTSETLIMITr, rval,
                                  PKTSETLIMITf);    
            min_thresh = drop_start * n / 100;

            /* Set the drop rate depending on the color */
            if (color == BCM_COSQ_DISCARD_COLOR_GREEN) {
                reg = CNG1COSDROPRATEr;
            } else {
                reg = CNG0COSDROPRATEr;
            }
            BCM_IF_ERROR_RETURN
                (soc_reg32_read(unit, soc_reg_addr
                                (unit, reg, local_port, i), &rval)); 
            encoding = _bcm_hu_api_val_to_hw_drop_prob(-1 * drop_slope);
            soc_reg_field_set(unit, reg, &rval, DROPRATEf, encoding);
            BCM_IF_ERROR_RETURN
                (soc_reg32_write(unit, soc_reg_addr
                                 (unit, reg, local_port, i), rval)); 
            /* Enable simple RED for the (port, cos) pair */
            BCM_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, SIMPLEREDCONFIGr, local_port,
                                        _hu_cosfld[i], 1));
            /* Set the drop threshold depending on the color */ 
            if (color == BCM_COSQ_DISCARD_COLOR_GREEN) {
                reg = CNGCOSPKTLIMIT1r;
                fld = CNGPKTSETLIMIT1f;
            } else {
                reg = CNGCOSPKTLIMIT0r;
                fld = CNGPKTSETLIMIT0f;
            }
            soc_reg_field_set(unit, reg, &rval, fld, min_thresh);
            BCM_IF_ERROR_RETURN
                (soc_reg32_write(unit, soc_reg_addr
                                 (unit, reg, local_port, i), rval)); 
        }
    }

    return BCM_E_NONE;
}

int
bcm_hu_cosq_discard_port_get(int unit, bcm_port_t port,
                             bcm_cos_queue_t cosq,
                             uint32 color,
                             int *drop_start,
                             int *drop_slope,
                             int *average_time)
{
    bcm_port_t local_port;
    bcm_pbmp_t pbmp;
    uint32 rval, n;

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_local_get(unit, port, &local_port));
        BCM_PBMP_PORT_SET(pbmp, local_port);
    } else {
        if (port == -1) {
            BCM_PBMP_ASSIGN(pbmp, PBMP_PORT_ALL(unit));
        } else if (!SOC_PORT_VALID(unit, port)) {
            return BCM_E_PORT;
        } else {
            BCM_PBMP_PORT_SET(pbmp, port);
        }
    }
    if (cosq < -1 || cosq >= _hu_num_cosq[unit]) {
        return BCM_E_PARAM;
    } else if (cosq == -1) {
        cosq = 0;
    }
   
    BCM_PBMP_ITER(pbmp, local_port) {
        /* Get the total HOL limit first */
        BCM_IF_ERROR_RETURN
            (READ_HOLCOSPKTSETLIMITr(unit, local_port, cosq, &rval));
        n = soc_reg_field_get(unit, HOLCOSPKTSETLIMITr, rval, PKTSETLIMITf);

        /* Get the simple RED mode */
        BCM_IF_ERROR_RETURN(READ_SIMPLEREDCONFIGr(unit, local_port, &rval));   
        if (soc_reg_field_get(unit, SIMPLEREDCONFIGr, rval,
                              _hu_cosfld[cosq])) {
            if (color == BCM_COSQ_DISCARD_COLOR_GREEN) {
                BCM_IF_ERROR_RETURN
                    (READ_CNGCOSPKTLIMIT1r(unit, port, cosq, &rval));
                *drop_start = soc_reg_field_get(unit, CNGCOSPKTLIMIT1r, rval, 
                                                CNGPKTSETLIMIT1f) * 100 / n; 
                BCM_IF_ERROR_RETURN
                    (READ_CNG1COSDROPRATEr(unit, port, cosq, &rval));
                *drop_slope = -1 * _bcm_hu_hw_drop_prob_to_api_val[rval];
            } else {
                BCM_IF_ERROR_RETURN
                    (READ_CNGCOSPKTLIMIT0r(unit, port, cosq, &rval));
                *drop_start = soc_reg_field_get(unit, CNGCOSPKTLIMIT0r, rval, 
                                                CNGPKTSETLIMIT0f) * 100 / n; 
                BCM_IF_ERROR_RETURN
                    (READ_CNG0COSDROPRATEr(unit, port, cosq, &rval));
                *drop_slope = -1 * _bcm_hu_hw_drop_prob_to_api_val[rval];
            }
        }
        break;
    } 
    return BCM_E_NONE;
}

STATIC int
_bcm_hu_cosq_config_set(int unit, int numq)
{
    int         ncells, nports, xq_per_port, cos;
    int         n, nreset, used_cells;
    int		scells, sreset;
    soc_port_t  port;
    uint32      val, limit_red, limit_yel;
    soc_mem_t   xq_mem;
    int		mmu_flow_percent;
    int		mmu_red_drop_percent;
    int		mmu_yellow_drop_percent;
    int		mmu_static_bytes;
    int		mmu_static_cells;
    int		mmu_static_percent;
    int		mmu_reset_bytes;
    int		mmu_reset_cells = 0;
    int		mmu_overcommit;
    int		mmu_overcommit_stack;
    uint16      dev_id;
    uint8       rev_id;

    ncells = soc_mem_index_count(unit, MMU_CBPDATA0m);
        /* These special variables are for Hawkeye only. 
            Those values are retrieved from HAWKEYE MMU setting document*/

    nports = NUM_ALL_PORT(unit) + 1;  /* Include cmic port as well */

    /* get mmu control config values */
    mmu_flow_percent = soc_property_get(unit,
					spn_MMU_FLOW_PERCENT,
					MMU_FLOW_PERCENT);
    mmu_red_drop_percent = soc_property_get(unit,
					    spn_MMU_RED_DROP_PERCENT,
					    MMU_RED_DROP_PERCENT);
    mmu_yellow_drop_percent = soc_property_get(unit,
					       spn_MMU_YELLOW_DROP_PERCENT,
					       MMU_YELLOW_DROP_PERCENT);
    mmu_static_bytes = soc_property_get(unit,
					spn_MMU_STATIC_BYTES,
					MMU_STATIC_BYTES);
    mmu_static_percent = soc_property_get(unit,
					  spn_MMU_STATIC_PERCENT,
					  MMU_STATIC_PERCENT);
    mmu_reset_bytes = soc_property_get(unit,
				       spn_MMU_RESET_BYTES,
				       MMU_RESET_BYTES);
    mmu_overcommit = soc_property_get(unit,
				      spn_MMU_OVERCOMMIT,
				      MMU_OVERCOMMIT);
    mmu_overcommit_stack = soc_property_get(unit,
					    spn_MMU_OVERCOMMIT_STACK,
					    MMU_OVERCOMMIT_STACK);

    /* sanity check config values */
    if (mmu_flow_percent <= 0 || mmu_flow_percent > 100) {
	mmu_flow_percent = 100;
    }
    if (mmu_red_drop_percent <= 0 || mmu_red_drop_percent > 100) {
	mmu_red_drop_percent = 100;
    }
    if (mmu_yellow_drop_percent <= 0 || mmu_yellow_drop_percent > 100) {
	mmu_yellow_drop_percent = 100;
    }
    if (mmu_static_percent <= 0 || mmu_static_percent > 100) {
	mmu_static_percent = MMU_STATIC_PERCENT;
    }
    if (mmu_overcommit <= 0) {
	mmu_overcommit = 1;
    }
    if (mmu_overcommit_stack <= 0) {
	mmu_overcommit_stack = mmu_overcommit;
    }
    mmu_static_cells = (mmu_static_bytes + MMU_CELL_SIZE - 1) / MMU_CELL_SIZE;
    mmu_reset_cells = (mmu_reset_bytes + MMU_CELL_SIZE - 1) / MMU_CELL_SIZE;

    /* calculate static limits first */
    if (mmu_static_cells > 0) {
	scells = mmu_static_cells;
	sreset = scells;		/* 100% */
    } else {
	scells = (ncells / nports / numq) * mmu_static_percent / 100;
	sreset = scells * 75 / 100;	/* 75% */
    }
    
    soc_cm_get_id(unit, &dev_id, &rev_id);
    if (dev_id == BCM56147_DEVICE_ID) {
        scells += 4;
        sreset = scells;
    }

    used_cells = scells * numq * nports;

    /* remaining cells are dynamic */
    n = ncells - used_cells;
    if (n <= 0) {
	n = 0;
    }
    nreset = n - mmu_reset_cells;
    if (nreset < (n * MMU_RESET_MIN_PERCENT / 100)) {
	nreset = n * MMU_RESET_MIN_PERCENT / 100;
    }
    if (nreset <= 0) {
	nreset = 1;
    }

    /* device wide dynamic limits */

    SOC_IF_ERROR_RETURN(READ_TOTALDYNCELLSETLIMITr(unit, &val));
    soc_reg_field_set(unit, TOTALDYNCELLSETLIMITr,
                      &val, TOTALDYNCELLSETLIMITf, n);
    SOC_IF_ERROR_RETURN(WRITE_TOTALDYNCELLSETLIMITr(unit, val));
    SOC_IF_ERROR_RETURN(READ_TOTALDYNCELLRESETLIMITr(unit, &val));
    soc_reg_field_set(unit, TOTALDYNCELLRESETLIMITr,
                      &val, TOTALDYNCELLRESETLIMITf, nreset);
    SOC_IF_ERROR_RETURN(WRITE_TOTALDYNCELLRESETLIMITr(unit, val));
 
    /* per-port dynamic limits */
    PBMP_ALL_ITER(unit, port) {

         if (dev_id == BCM56147_DEVICE_ID) {
         /* For Hurricane(56147 variant), the value of field DYNCELLLIMIT 
         * of register DYNCELLLIMIT is 8448 and DYNCELLRESETLIMIT is 8424, 
         * refer hurricane_mmu_init for detailed computation */
        
            SOC_IF_ERROR_RETURN(READ_DYNCELLLIMITr(unit, port, &val));
            if (soc_reg_field_valid(unit, DYNCELLLIMITr, DYNCELLLIMITf)) {
                soc_reg_field_set(unit, DYNCELLLIMITr, &val,
                                  DYNCELLLIMITf, 8448);
            }

            if (soc_reg_field_valid(unit, DYNCELLLIMITr, DYNCELLRESETLIMITf)) {
                soc_reg_field_set(unit, DYNCELLLIMITr, &val,
                                  DYNCELLRESETLIMITf, 8424);
            }
        }
	else {
        /* For other variants use 6040 and 6016 */
            SOC_IF_ERROR_RETURN(READ_DYNCELLLIMITr(unit, port, &val));
            if (soc_reg_field_valid(unit, DYNCELLLIMITr, DYNCELLLIMITf)) {
                soc_reg_field_set(unit, DYNCELLLIMITr, &val,
                                  DYNCELLLIMITf, 6040);
            }

            if (soc_reg_field_valid(unit, DYNCELLLIMITr, DYNCELLRESETLIMITf)) {
                soc_reg_field_set(unit, DYNCELLLIMITr, &val,
                                  DYNCELLRESETLIMITf, 6016);
            }
	}


        SOC_IF_ERROR_RETURN(WRITE_DYNCELLLIMITr(unit, port, val));


        /*
         * Number of transactions allocated to each port,
         * shared between that port's active cosqs.
         */
        SOC_IF_ERROR_RETURN(soc_hu_xq_mem(unit, port, &xq_mem));
        xq_per_port = soc_mem_index_count(unit, xq_mem);


        /*
         * Divide weighted per-port packet limits into each cos.
         * Initialize all COSQs, even the inactive ones.
         */
        for (cos = 0; cos < NUM_COS(unit); cos++) {
	    /* set per-port static limits (calculated earlier) */
            SOC_IF_ERROR_RETURN
                (READ_LWMCOSCELLSETLIMITr(unit, port, cos, &val));
            if (soc_reg_field_valid(unit, LWMCOSCELLSETLIMITr,
                                    CELLSETLIMITf)) {
                soc_reg_field_set(unit, LWMCOSCELLSETLIMITr,
                                  &val, CELLSETLIMITf,
				  cos < numq ? scells : 0);
            }

            if (soc_reg_field_valid(unit, LWMCOSCELLSETLIMITr,
                                    CELLRESETLIMITf)) {
                soc_reg_field_set(unit, LWMCOSCELLSETLIMITr,
                                  &val, CELLRESETLIMITf,
				  cos < numq ? sreset : 0);
            }
            SOC_IF_ERROR_RETURN
                (WRITE_LWMCOSCELLSETLIMITr(unit, port, cos, val));


	    /* set red and yellow drop limits */
	    /* set as a percentage of the green drop (HOL) limit */

            SOC_IF_ERROR_RETURN
		(READ_HOLCOSPKTSETLIMITr(unit, port, cos, &val));
            n = soc_reg_field_get(unit, HOLCOSPKTSETLIMITr, val,
                                    PKTSETLIMITf);

            /* Make sure numq will work with the cosq weighting. */
            if ((cos < numq) && (n == 4)) {
                /*
                 * This means that this COSQ has been
                 * disabled due to XQ weighting, but
                 * has been selected as active. Config error.
                 */
                return BCM_E_PARAM;
            }

	    /* per-queue packet red and yellow drop */
            if (cos < numq) {
                limit_red = n * mmu_red_drop_percent / 100;
                limit_yel = n * mmu_yellow_drop_percent / 100;
            } else {
                limit_red = xq_per_port - 1; /* Max limit (disabled) */
                limit_yel = xq_per_port - 1; /* Max limit (disabled) */
            }

 /* Granularity limitation */
                limit_red /= BCM_HU_CNG_PKT_LIMIT_GRANULARITY;
                limit_yel /= BCM_HU_CNG_PKT_LIMIT_GRANULARITY;
            

            val = 0;
            soc_reg_field_set(unit, CNGCOSPKTLIMIT0r, &val,
                              CNGPKTSETLIMIT0f, limit_red);
	    SOC_IF_ERROR_RETURN(WRITE_CNGCOSPKTLIMIT0r(unit, port, cos, val));

            val = 0;
            soc_reg_field_set(unit, CNGCOSPKTLIMIT1r, &val,
                              CNGPKTSETLIMIT1f, limit_yel);
	    SOC_IF_ERROR_RETURN(WRITE_CNGCOSPKTLIMIT1r(unit, port, cos, val));


        }


        if (dev_id == BCM56147_DEVICE_ID) {
            SOC_IF_ERROR_RETURN(READ_IBPCELLSETLIMITr(unit, port, &val));

            /* set cell limit = 36 as recommended by design team */
            soc_reg_field_set(unit, IBPCELLSETLIMITr, &val,
                              CELLSETLIMITf, 36);
            SOC_IF_ERROR_RETURN(WRITE_IBPCELLSETLIMITr(unit, port, val));

            SOC_IF_ERROR_RETURN(READ_IBPPKTSETLIMITr(unit, port, &val));
            /* set pkt set limit = 33 as recommended by design team */  
            soc_reg_field_set(unit, IBPPKTSETLIMITr, &val,
                              PKTSETLIMITf, 33);
            soc_reg_field_set(unit, IBPPKTSETLIMITr, &val,
                              RESETLIMITSELf, 0);	
            SOC_IF_ERROR_RETURN(WRITE_IBPPKTSETLIMITr(unit, port, val));
        }
        else {
            SOC_IF_ERROR_RETURN(READ_IBPCELLSETLIMITr(unit, port, &val));

            /* set cell limit = 80 (9728/128 +4); 9728 is max pkt size and 128 is cell size ; 
             * as recommended by design team */
            soc_reg_field_set(unit, IBPCELLSETLIMITr, &val,
                              CELLSETLIMITf, 80);
            SOC_IF_ERROR_RETURN(WRITE_IBPCELLSETLIMITr(unit, port, val));

            SOC_IF_ERROR_RETURN(READ_IBPPKTSETLIMITr(unit, port, &val));
            /* set pkt set limit = 12 as recommended by design team */  
            soc_reg_field_set(unit, IBPPKTSETLIMITr, &val,
                              PKTSETLIMITf, 12);
            soc_reg_field_set(unit, IBPPKTSETLIMITr, &val,
                              RESETLIMITSELf, 0);	
            SOC_IF_ERROR_RETURN(WRITE_IBPPKTSETLIMITr(unit, port, val));
        }

        SOC_IF_ERROR_RETURN(READ_IBPDISCARDSETLIMITr(unit, port, &val));
        soc_reg_field_set(unit, IBPDISCARDSETLIMITr, &val,
                          DISCARDSETLIMITf, ncells-1);
        SOC_IF_ERROR_RETURN(WRITE_IBPDISCARDSETLIMITr(unit, port, val));

    }

    /* Overall MMU controls */
    SOC_IF_ERROR_RETURN(READ_MISCCONFIGr(unit, &val));
    soc_reg_field_set(unit, MISCCONFIGr, &val, DYNAMIC_MEMORY_ENf, 1);

    /* set skidmarker to 3 */
    soc_reg_field_set(unit, MISCCONFIGr, &val, SKIDMARKERf, 3);


    if (soc_reg_field_valid(unit, MISCCONFIGr, HOL_CELL_SOP_DROP_ENf)) {
        soc_reg_field_set(unit, MISCCONFIGr, &val, HOL_CELL_SOP_DROP_ENf, 1);
    }


    SOC_IF_ERROR_RETURN(WRITE_MISCCONFIGr(unit, val));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_cosq_mapping_set
 * Purpose:
 *      Set which cosq a given priority should fall into
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      priority - Priority value to map
 *      cosq - COS queue to map to
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_hu_cosq_mapping_set(int unit, bcm_port_t port,
                        bcm_cos_t priority, bcm_cos_queue_t cosq)
{
    uint32 val, old_index, new_index;
    int i;
    port_cos_map_entry_t cos_map_array[HU_COS_MAP_ENTRIES_PER_SET], *entry_p;
    void *entries[HU_COS_MAP_ENTRIES_PER_SET];
    bcm_pbmp_t ports;
    bcm_port_t local_port;

    if (priority < 0 || priority >= 8) {
        return (BCM_E_PARAM);
    }

    if (cosq < 0 || cosq >= 8) {
        return (BCM_E_PARAM);
    }

    if (port == -1) {	/* all ports */
        BCM_PBMP_ASSIGN(ports, PBMP_ALL(unit));
    } else {
        if (BCM_GPORT_IS_SET(port)) {
            BCM_IF_ERROR_RETURN(
                bcm_esw_port_local_get(unit, port, &local_port)); 
        } else {
            local_port = port;
        }

        if (!SOC_PORT_VALID(unit, local_port) || !IS_ALL_PORT(unit, local_port)) {
            return BCM_E_PORT;
        }
        BCM_PBMP_CLEAR(ports);
        BCM_PBMP_PORT_ADD(ports, local_port);
    }

    PBMP_ITER(ports, local_port) {
        SOC_IF_ERROR_RETURN(READ_COS_MAP_SELr(unit, local_port, &val));
        old_index = soc_reg_field_get(unit, COS_MAP_SELr, val, SELECTf);
        old_index *= HU_COS_MAP_ENTRIES_PER_SET;

        /* get current mapping profile values */
        for (i = 0; i < HU_COS_MAP_ENTRIES_PER_SET; i++) {
            entry_p = SOC_PROFILE_MEM_ENTRY(unit, _hu_cos_map_profile[unit],
                                            port_cos_map_entry_t *, (old_index + i));
            memcpy(&cos_map_array[i], entry_p, sizeof(*entry_p));
            entries[i] = (void *) &cos_map_array[i];
        }
        soc_mem_field32_set(unit, PORT_COS_MAPm,
                            &cos_map_array[priority], COSf, cosq);
        SOC_IF_ERROR_RETURN
            (soc_profile_mem_add(unit, _hu_cos_map_profile[unit],
                                 (void *) &entries,
                                 HU_COS_MAP_ENTRIES_PER_SET, &new_index));
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, COS_MAP_SELr, local_port, 
                                    SELECTf,
                                    new_index / HU_COS_MAP_ENTRIES_PER_SET));
        SOC_IF_ERROR_RETURN
            (soc_profile_mem_delete(unit, _hu_cos_map_profile[unit], old_index));

#ifndef BCM_COSQ_HIGIG_MAP_DISABLE
        if (IS_HG_PORT(unit, local_port) || local_port == CMIC_PORT(unit)) {
            SOC_IF_ERROR_RETURN
                (soc_profile_mem_add(unit, _hu_cos_map_profile[unit],
                                     (void *) &entries,
                                     HU_COS_MAP_ENTRIES_PER_SET, &new_index));
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, ICOS_MAP_SELr, local_port, 
                                        SELECTf,
                                        new_index / HU_COS_MAP_ENTRIES_PER_SET));
            SOC_IF_ERROR_RETURN
                (soc_profile_mem_delete(unit, _hu_cos_map_profile[unit], old_index));
        }
#endif
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_cosq_mapping_get
 * Purpose:
 *      Determine which COS queue a given priority currently maps to.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      priority - Priority value
 *      cosq - (Output) COS queue number
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_hu_cosq_mapping_get(int unit, bcm_port_t port,
                        bcm_cos_t priority, bcm_cos_queue_t *cosq)
{
    uint32 val;
    int index;
    port_cos_map_entry_t *entry_p;
    bcm_port_t local_port;

    if (priority < 0 || priority >= 8) {
        return (BCM_E_PARAM);
    }

    if (port == -1) {
        local_port = REG_PORT_ANY;
    } else {
        if (BCM_GPORT_IS_SET(port)) {
            BCM_IF_ERROR_RETURN(
                bcm_esw_port_local_get(unit, port, &local_port)); 
        } else {
            local_port = port;
        }

        if (!SOC_PORT_VALID(unit, local_port) || !IS_ALL_PORT(unit, local_port)) {
            return BCM_E_PORT;
        }
    }

    SOC_IF_ERROR_RETURN(READ_COS_MAP_SELr(unit, local_port, &val));
    index = soc_reg_field_get(unit, COS_MAP_SELr, val, SELECTf);
    index *= HU_COS_MAP_ENTRIES_PER_SET;

    entry_p = SOC_PROFILE_MEM_ENTRY(unit, _hu_cos_map_profile[unit],
                                    port_cos_map_entry_t *, 
                                    (index + priority));
    *cosq = soc_mem_field32_get(unit, PORT_COS_MAPm, entry_p, COSf);
    return BCM_E_NONE;
}

STATIC int
_bcm_hu_cosq_gport_bandwidth_port_resolve(int unit, bcm_gport_t gport,
                                          bcm_cos_queue_t cosq,
                                          bcm_port_t *local_port,
                                          bcm_cos_queue_t *cosq_start,
                                          bcm_cos_queue_t *cosq_end)
{
    bcm_module_t modid;
    bcm_trunk_t trunk_id;	

    if (BCM_GPORT_IS_SET(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_hu_cosq_resolve_mod_port(unit, gport, &modid,
                                           local_port, &trunk_id, local_port));
    } else {
        *local_port = gport;
        if (!SOC_PORT_VALID(unit, gport)) {
            return BCM_E_PORT;
        }
    }

    /* hurricane does not have vlan queues and S1 scheduler*/
    if (cosq >= _hu_num_cosq[unit]) {
        return BCM_E_PARAM;
    } else if (cosq < 0) {
        *cosq_start = 0;
        *cosq_end = 7;
    } else {
        *cosq_start = *cosq_end = cosq;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_hu_cosq_gport_bandwidth_burst_get
 * Purpose:
 *
 * Parameters:
 *      unit - (IN) Unit number.
 *      gport - (IN) GPORT ID.
 *      cosq - (IN) COS queue to configure, -1 for all COS queues.
 *      kbits_burst - (OUT) maximum burst, kbits.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_hu_cosq_gport_bandwidth_burst_get(int unit, bcm_gport_t gport,
                                      bcm_cos_queue_t cosq,
                                      uint32 *kbits_burst)
{
    bcm_port_t local_port, port;
    bcm_cos_queue_t cosq_start = 0, cosq_end = 0;
    uint32 kbits_sec_min, kbits_sec_max, flags;    /* Dummy variables */

    if (gport < 0) {
        port = SOC_PORT_MIN(unit,all);

        BCM_IF_ERROR_RETURN
            (_bcm_hu_cosq_gport_bandwidth_port_resolve(unit, port, cosq,
                                                       &local_port,
                                                       &cosq_start, &cosq_end));
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_hu_cosq_gport_bandwidth_port_resolve(unit, gport, cosq,
                                                       &local_port,
                                                       &cosq_start, &cosq_end));
    }

    kbits_sec_min = kbits_sec_max = flags = 0;

    BCM_IF_ERROR_RETURN
        (bcm_hu_cosq_port_bandwidth_get(unit, local_port, cosq_start,
                                        &kbits_sec_min, &kbits_sec_max,
                                        kbits_burst, &flags));
    return BCM_E_NONE;
}

/*  
 * Function:
 *      bcm_hu_cosq_gport_bandwidth_burst_set
 * Purpose:
 *       
 * Parameters:
 *      unit - (IN) Unit number.
 *      gport - (IN) GPORT ID.
 *      cosq - (IN) COS queue to configure, -1 for all COS queues.
 *      kbits_burst - (IN) maximum burst, kbits.
 * Returns:
 *      BCM_E_XXX 
 */ 
int
bcm_hu_cosq_gport_bandwidth_burst_set(int unit, bcm_gport_t gport,
                                      bcm_cos_queue_t cosq,
                                      uint32 kbits_burst)
{
    bcm_port_t local_port, port;
    int i;
    bcm_cos_queue_t cosq_start = 0, cosq_end = 0;
    uint32 kbits_sec_min, kbits_sec_max, flags;
    uint32 kbits_sec_burst; /* Dummy variable */

    if (gport < 0) {
        PBMP_ALL_ITER(unit, port) {
            BCM_IF_ERROR_RETURN
                (_bcm_hu_cosq_gport_bandwidth_port_resolve(unit, port, cosq,
                                                           &local_port,
                                                           &cosq_start, &cosq_end));
            for (i = cosq_start; i <= cosq_end; i++) {
                BCM_IF_ERROR_RETURN
                    (bcm_hu_cosq_port_bandwidth_get(unit, local_port, i,
                                                    &kbits_sec_min, &kbits_sec_max,
                                                    &kbits_sec_burst, &flags));
                BCM_IF_ERROR_RETURN
                    (bcm_hu_cosq_port_bandwidth_set(unit, local_port, i,
                                                    kbits_sec_min,
                                                    kbits_sec_max,
                                                    kbits_burst,
                                                    flags));
            }
        }
        return BCM_E_NONE;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_hu_cosq_gport_bandwidth_port_resolve(unit, gport, cosq,
                                                   &local_port,
                                                   &cosq_start, &cosq_end));

    for (i = cosq_start; i <= cosq_end; i++) {
        BCM_IF_ERROR_RETURN
            (bcm_hu_cosq_port_bandwidth_get(unit, local_port, cosq_start,
                                            &kbits_sec_min, &kbits_sec_max,
                                            &kbits_sec_burst, &flags));
        BCM_IF_ERROR_RETURN
            (bcm_hu_cosq_port_bandwidth_set(unit, local_port, i,
                                            kbits_sec_min,
                                            kbits_sec_max,
                                            kbits_burst,
                                            flags));
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_hu_cosq_sched_weight_max_get
 * Purpose:
 *      Retrieve maximum weights for given COS policy.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      mode - Scheduling mode, one of BCM_COSQ_xxx
 *      weight_max - (output) Maximum weight for COS queue.
 *              For DRR mode Weight is specified in Kbytes
 *              0 if mode is BCM_COSQ_STRICT.
 *              1 if mode is BCM_COSQ_ROUND_ROBIN.
 *              -1 if not applicable to mode.
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_hu_cosq_sched_weight_max_get(int unit, int mode, int *weight_max)
{
    switch (mode) {
    case BCM_COSQ_STRICT:
        *weight_max = BCM_COSQ_WEIGHT_STRICT;
        break;
    case BCM_COSQ_ROUND_ROBIN:
        *weight_max = BCM_COSQ_WEIGHT_MIN;
        break;
    case BCM_COSQ_WEIGHTED_ROUND_ROBIN:
        *weight_max = HU_WRR_WEIGHT_MAX;
        break;
    case BCM_COSQ_DEFICIT_ROUND_ROBIN:
        /*max weight in kb will be
        max weight * max value of MTU quanta sel */
        *weight_max = HU_DRR_WEIGHT_MAX * MTU_HU_Quanta[BCM_HU_MTU_QUANTA_ID_COUNT-1]; 
        break;
    default:
        *weight_max = BCM_COSQ_WEIGHT_UNLIMITED;
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     bcm_hu_cosq_sw_dump
 * Purpose:
 *     Displays COS Queue information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
bcm_hu_cosq_sw_dump(int unit)
{
    int  i;

    soc_cm_print("\nSW Information COSQ - Unit %d\n", unit);

    /* Number COSQ */
    soc_cm_print("    Number: %d\n", _hu_num_cosq[unit]);

    /* COSQ Map profile */
    /* Display those entries with reference count > 0 */
    soc_cm_print("    COSQ Map Profile:\n");
    if (_hu_cos_map_profile[unit] != NULL) {
        int     num_entries;
        int     ref_count;
        int     entries_per_set;
        uint32  index;
        port_cos_map_entry_t *entry_p;
        uint32  cosq, hg_cosq;

        num_entries = soc_mem_index_count
            (unit, _hu_cos_map_profile[unit]->tables[0].mem);
        soc_cm_print("        Number of entries: %d\n", num_entries);
        soc_cm_print("        Index RefCount EntriesPerSet - "
                     "COS HG_COS\n");

        for (index = 0; index < num_entries;
             index += HU_COS_MAP_ENTRIES_PER_SET) {
            if (SOC_FAILURE
                (soc_profile_mem_ref_count_get(unit,
                                               _hu_cos_map_profile[unit],
                                               index, &ref_count))) {
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            for (i = 0; i < HU_COS_MAP_ENTRIES_PER_SET; i++) {
                entries_per_set =
                    _hu_cos_map_profile[unit]->tables[0].entries[index+i].entries_per_set;
                soc_cm_print("       %5d %8d %13d    ",
                             index + i, ref_count, entries_per_set);

                entry_p = SOC_PROFILE_MEM_ENTRY(unit,
                                                _hu_cos_map_profile[unit],
                                                port_cos_map_entry_t *,
                                                index + i);
                cosq = soc_mem_field32_get(unit,
                                           PORT_COS_MAPm,
                                           entry_p,
                                           COSf);
                soc_cm_print("%2d ", cosq);
                if (soc_mem_field_valid(unit, PORT_COS_MAPm, HG_COSf)) {
                    hg_cosq = soc_mem_field32_get(unit, PORT_COS_MAPm,
                                                  entry_p,
                                                  HG_COSf);
                    soc_cm_print("   %2d", hg_cosq);
                }
                soc_cm_print("\n");
            }
        }
    }

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#else /* BCM_HURRICANE_SUPPORT */
int _hu_cosq_not_empty;
#endif  /* BCM_HURRICANE_SUPPORT */


