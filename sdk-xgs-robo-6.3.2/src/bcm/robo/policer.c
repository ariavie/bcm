/*
 * $Id: policer.c 1.10 Broadcom SDK $
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
 * File:    policer.c
 * Purpose: Manages policer functions
 *
 * Note : Not for RoboSwitch currently.
 */

#include <bcm/error.h>
#include <bcm/types.h>
#include <bcm/policer.h>
#include <bcm/debug.h>
#include <bcm_int/robo/field.h>
#include <sal/core/sync.h>
#include <sal/core/libc.h>
#include <sal/core/alloc.h>
#include <soc/drv.h>


#define BCM_ROBO_NUM_POLICER    2

#define POLICER_OUT(flags, stuff)  BCM_DEBUG(flags | BCM_DBG_POLICER, stuff)
#define POLICER_WARN(stuff)        BCM_DEBUG(BCM_DBG_WARN, stuff)
#define POLICER_ERR(stuff)         BCM_DEBUG(BCM_DBG_ERR, stuff)
#define POLICER_VERB(stuff)        POLICER_OUT(BCM_DBG_VERBOSE, stuff)
#define POLICER_VVERB(stuff)       POLICER_OUT(BCM_DBG_VVERBOSE, stuff)


#define POLICER_UNIT_CHECK(unit) \
do { \
    if (!((unit) >= 0 && ((unit) < (BCM_MAX_NUM_UNITS)))) { \
        POLICER_ERR(("%s: Invalid unit \n", FUNCTION_NAME())); \
        return BCM_E_UNIT; \
    } \
} while (0)


#define POLICER_INIT_CHECK(unit) \
do { \
    POLICER_UNIT_CHECK(unit); \
    if (!_policer[unit].lock) { \
        POLICER_ERR(("%s: Policers unitialized on unit:%d \n", \
                     FUNCTION_NAME(), unit)); \
        return BCM_E_INIT; \
    } \
} while (0)

#define POLICER_UNIT_LOCK_nr(unit, rv) \
do { \
    if (sal_mutex_take(_policer[unit].lock, sal_mutex_FOREVER)) { \
        POLICER_ERR(("%s: sal_mutex_take failed for unit %d. \n", \
                    FUNCTION_NAME(), unit)); \
        (rv) = BCM_E_INTERNAL; \
    } \
} while (0)

#define POLICER_UNIT_LOCK(unit) \
do { \
    int pol__rv__ = BCM_E_NONE; \
    POLICER_UNIT_LOCK_nr(unit, pol__rv__);  \
    if (BCM_FAILURE(pol__rv__)) { return pol__rv__; } \
} while (0)

#define POLICER_UNIT_UNLOCK_nr(unit, rv) \
do { \
    if (sal_mutex_give(_policer[unit].lock)) { \
        POLICER_ERR(("%s: sal_mutex_give failed for unit %d. \n", \
                    FUNCTION_NAME(), unit)); \
        (rv) = BCM_E_INTERNAL; \
    } \
} while (0)

#define POLICER_UNIT_UNLOCK(unit) \
do { \
    int pol__rv__ = BCM_E_NONE; \
    POLICER_UNIT_UNLOCK_nr(unit, pol__rv__);  \
    if (BCM_FAILURE(pol__rv__)) { return pol__rv__; } \
} while (0)


typedef struct _bcm_policer_glob_s {
    sal_mutex_t                 lock;      /* per unit lock */
    int                         pol_count; /* number of created policers */
    bcm_policer_config_t    *policers;
    int                         *policers_used; /* management */
} _bcm_policer_glob_t;


#if defined(BCM_53101)
static _bcm_policer_glob_t _policer[BCM_MAX_NUM_UNITS]; /* glob policer data */
static sal_mutex_t         _policer_glob_lock;          /* glob policer lock */
#endif


int 
bcm_robo_policer_init(int unit)
{
    if (SOC_IS_LOTUS(unit)) {
#if defined(BCM_53101)
        int             status = BCM_E_NONE;
        sal_mutex_t     local_lock = NULL;
        
        if (_policer[unit].lock) {
            /* status = bcm_robo_policer_detach(unit); */
            BCM_IF_ERROR_RETURN(status);
            /* _policer_glob_lock = NULL; */
        }

        if (!_policer_glob_lock) {
            /* create and initialize the global lock */
            local_lock = sal_mutex_create("_policer_glob_lock");
            if (!local_lock) {
                POLICER_ERR(("%s: sal_mutex_create failed. \n", FUNCTION_NAME()));
                return BCM_E_RESOURCE;
            }
            if (sal_mutex_take(local_lock, sal_mutex_FOREVER) == 0) {
                /* initialize the global struct */
                sal_memset(&(_policer), 0, sizeof(_policer));
                if (sal_mutex_give(local_lock) != 0) {
                    POLICER_ERR(("%s: sal_mutex_give failed.\n", FUNCTION_NAME()));
                    sal_mutex_destroy(local_lock);
                    return BCM_E_INTERNAL;
                }
            } else {
                POLICER_ERR(("%s: sal_mutex_take failed. \n", FUNCTION_NAME()));
                sal_mutex_destroy(local_lock);
                return BCM_E_INTERNAL;
            }
            _policer_glob_lock = local_lock;
        }

        /* get global lock */
        if (sal_mutex_take(_policer_glob_lock, sal_mutex_FOREVER) != 0) {
            POLICER_ERR(("%s: sal_mutex_take failed. \n", FUNCTION_NAME()));
            sal_mutex_destroy(_policer_glob_lock); 
            return BCM_E_INTERNAL;
        }


        if (_policer[unit].lock == 0) {
            /* Now initialize the per unit policer data */
            local_lock = sal_mutex_create("_policer_unit_lock");
            if (local_lock) {
                _policer[unit].policers = sal_alloc((sizeof(bcm_policer_config_t) * 
                                                    BCM_ROBO_NUM_POLICER),
                                                     "_policer_unit_policer");
                if (_policer[unit].policers == 0) {
                    status = BCM_E_MEMORY;
                }

                _policer[unit].policers_used = sal_alloc((sizeof(int) * 
                                                    BCM_ROBO_NUM_POLICER),
                                                     "_policer_unit_policer_used");
                if (_policer[unit].policers_used == 0) {
                    sal_free(_policer[unit].policers);
                    status = BCM_E_MEMORY;
                }
            } else {
                POLICER_ERR(("%s: sal_mutex_create failed. \n", FUNCTION_NAME()));
                status = BCM_E_RESOURCE;
            }

            if (status == BCM_E_NONE) {
                if (sal_mutex_take(local_lock, sal_mutex_FOREVER) == 0) {
                    _policer[unit].lock = local_lock;
                    if (sal_mutex_give(local_lock) != 0) {
                        POLICER_ERR(("%s: sal_mutex_give failed for unit %d. \n",
                            FUNCTION_NAME(), unit));
                        sal_mutex_destroy(local_lock);
                        _policer[unit].lock = NULL;
                        status = BCM_E_INTERNAL;
                    }
                } else {
                    POLICER_ERR(("%s: sal_mutex_take failed \n", FUNCTION_NAME()));
                    sal_mutex_destroy(local_lock);
                    status = BCM_E_INTERNAL;
                }
            } else {
                /* Destroy Mutex */
                if (local_lock) {
                    sal_mutex_destroy(local_lock);
                }
            }
        } else {
            /* lock already created...meaning initialized before */
            if (sal_mutex_take(_policer[unit].lock, sal_mutex_FOREVER)) {
                POLICER_ERR(("%s: sal_mutex_take failed for unit %d. \n",
                            FUNCTION_NAME(), unit));
                status = BCM_E_INTERNAL; \
            }

            sal_memset(_policer[unit].policers, 0, (sizeof(bcm_policer_config_t) * 
                                                    BCM_ROBO_NUM_POLICER));
            sal_memset(_policer[unit].policers_used, 0, (sizeof(int) * 
                                                    BCM_ROBO_NUM_POLICER));
            if (sal_mutex_give(_policer[unit].lock) != 0) {
                POLICER_ERR(("%s: sal_mutex_give failed for unit %d. \n",
                    FUNCTION_NAME(), unit));
                sal_mutex_destroy(_policer[unit].lock);
                _policer[unit].lock = NULL;
                status = BCM_E_INTERNAL;
            }
            
            
            
            
            /* POLICER_ERR(("%s: failed. Previously initialized\n", FUNCTION_NAME())); */
            /* status = BCM_E_INTERNAL; */
        }
        

        if (sal_mutex_give(_policer_glob_lock) != 0) {
            POLICER_ERR(("%s: sal_mutex_give failed. \n", FUNCTION_NAME()));
            status = BCM_E_INTERNAL;
        }

        
        return status; 
#endif
        }

    return BCM_E_NONE;    

}

int 
bcm_robo_policer_create(int unit, bcm_policer_config_t *pol_cfg, 
                        bcm_policer_t *policer_id)
{
    if (SOC_IS_LOTUS(unit)) {
#if defined(BCM_53101)
        int rv = BCM_E_NONE;
        int i;

        POLICER_INIT_CHECK(unit);

        if ((pol_cfg == NULL) || (policer_id == NULL)) {
            return BCM_E_PARAM;
        }

        /* 
         * Check the policer mode
         * Onlu support bcmPolicerModeCommitted for robo chips
         */
        
        if (pol_cfg->mode !=  bcmPolicerModeCommitted) {
            return BCM_E_UNAVAIL;
        }

        /* Need check flags ? */

        POLICER_UNIT_LOCK(unit);
        if (pol_cfg->flags & BCM_POLICER_WITH_ID) {
            /* policer_id contains the requested id, validate it */
            if ((*policer_id <= 0) || (*policer_id >= BCM_ROBO_NUM_POLICER)) {
                POLICER_ERR(("Invalid policer id specified.\n"));
                POLICER_UNIT_UNLOCK(unit);
                return BCM_E_CONFIG;
            }
            if (_policer[unit].policers_used[(*policer_id -1)]) {
                POLICER_ERR(("Faild to reserve policer id specified.\n"));
                POLICER_UNIT_UNLOCK(unit);
                return BCM_E_CONFIG;
            }
        } else {
            /* auto assign a policer id */
            for (i =0; i < BCM_ROBO_NUM_POLICER; i++) {
                if (_policer[unit].policers_used[i] == 0) {
                    *policer_id = i +1;
                    _policer[unit].pol_count++;
                    _policer[unit].policers_used[i] = 1;
                    break;
                }
            }
            if (i == BCM_ROBO_NUM_POLICER) {
                POLICER_ERR(("%s: Out of policer. \n", 
                             FUNCTION_NAME()));
                POLICER_UNIT_UNLOCK(unit);
                return BCM_E_RESOURCE;
            }
        }

        sal_memcpy(&_policer[unit].policers[(*policer_id) -1], pol_cfg, 
            sizeof(bcm_policer_config_t));
        
        POLICER_UNIT_UNLOCK(unit);
        return rv; 
#endif
    } 
#ifdef BCM_FIELD_SUPPORT 
    if (soc_feature(unit, soc_feature_field)) {
        return _bcm_robo_field_policer_create(unit, pol_cfg, 0, policer_id);
    }
#endif
    return BCM_E_UNAVAIL;

}

int 
bcm_robo_policer_destroy(int unit, bcm_policer_t policer_id)
{
    if (SOC_IS_LOTUS(unit)) {
#if defined(BCM_53101)
        POLICER_INIT_CHECK(unit);

        if (policer_id == 0) {
            return BCM_E_PARAM;
        }

        POLICER_UNIT_LOCK(unit);

        if (_policer[unit].policers_used[(policer_id - 1)]) {
            sal_memset(&_policer[unit].policers[(policer_id -1)], 0,
                sizeof(bcm_policer_config_t));
            _policer[unit].pol_count--;
            _policer[unit].policers_used[(policer_id - 1)] = 0;
        }

        POLICER_UNIT_UNLOCK(unit);

        return BCM_E_NONE; 
#endif
    }
#ifdef BCM_FIELD_SUPPORT 
    if (soc_feature(unit, soc_feature_field)) {
        return _bcm_robo_field_policer_destroy(unit, policer_id);
    }
#endif
    return BCM_E_UNAVAIL;

}

int 
bcm_robo_policer_destroy_all(int unit)
{    
    if (SOC_IS_LOTUS(unit)) {
#if defined(BCM_53101)
        int i;
        int rv = BCM_E_NONE;
        
        for (i=0; i < BCM_ROBO_NUM_POLICER; i++) {
            rv = bcm_robo_policer_destroy(unit, i+1);
            if (rv != BCM_E_NONE) {
                POLICER_ERR(("%s: failer to destroy the policer %d. \n", 
                             FUNCTION_NAME(), i+1));
                return BCM_E_RESOURCE;
            }
        }

        return rv;
#endif
    }
#ifdef BCM_FIELD_SUPPORT 
    if (soc_feature(unit, soc_feature_field)) {
        return _bcm_robo_field_policer_destroy_all(unit);
    }
#endif
    return BCM_E_UNAVAIL;
}
int 
bcm_robo_policer_set(int unit, bcm_policer_t policer_id, 
                     bcm_policer_config_t *pol_cfg)
{
    if (SOC_IS_LOTUS(unit)) {
#if defined(BCM_53101)
        int rv = BCM_E_NONE;

        POLICER_INIT_CHECK(unit);

        if (policer_id == 0) {
            return BCM_E_PARAM;
        }

        POLICER_UNIT_LOCK(unit);

        if (!(_policer[unit].policers_used[(policer_id -1)])) {
            POLICER_UNIT_UNLOCK(unit);
            return BCM_E_NOT_FOUND;
        }

        sal_memcpy(&(_policer[unit].policers[policer_id -1]), pol_cfg,
            sizeof(bcm_policer_config_t));
        
        POLICER_UNIT_UNLOCK(unit);
        return rv;
#endif
    }
#ifdef BCM_FIELD_SUPPORT 
    if (soc_feature(unit, soc_feature_field)) {
        return _bcm_robo_field_policer_set(unit, policer_id, pol_cfg);
    }
#endif
    return BCM_E_UNAVAIL;

}

int 
bcm_robo_policer_get(int unit, bcm_policer_t policer_id, 
                     bcm_policer_config_t *pol_cfg)
{
    if (SOC_IS_LOTUS(unit)) {
#if defined(BCM_53101)
        int rv = BCM_E_NONE;

        POLICER_INIT_CHECK(unit);

        if (policer_id == 0) {
            return BCM_E_PARAM;
        }

        POLICER_UNIT_LOCK(unit);

        if (!(_policer[unit].policers_used[(policer_id -1)])) {
            POLICER_UNIT_UNLOCK(unit);
            return BCM_E_NOT_FOUND;
        }

        sal_memcpy(pol_cfg, &(_policer[unit].policers[(policer_id) -1]),
            sizeof(bcm_policer_config_t));
        
        POLICER_UNIT_UNLOCK(unit);
        return rv; 
#endif
    }
#ifdef BCM_FIELD_SUPPORT 
    if (soc_feature(unit, soc_feature_field)) {
        return _bcm_robo_field_policer_get(unit, policer_id, pol_cfg);
    }
#endif
    return BCM_E_UNAVAIL;

}

int 
bcm_robo_policer_traverse(int unit, bcm_policer_traverse_cb traverse_callback, 
                          void *cookie)
{
    if (SOC_IS_LOTUS(unit)) {
#if defined(BCM_53101)
        int                     idx, rv;
        bcm_policer_config_t    *pol_cfg = NULL;

        rv = BCM_E_NONE;
        POLICER_INIT_CHECK(unit);

        if (!traverse_callback) {
            POLICER_ERR(("%s: Invalid callback function \n", FUNCTION_NAME()));
            return BCM_E_PARAM;
        }

        if (!(pol_cfg = (bcm_policer_config_t *)
                        sal_alloc(sizeof(bcm_policer_config_t), "pol_cfg"))) {
            return BCM_E_MEMORY;
        }

        POLICER_UNIT_LOCK_nr(unit, rv);
        
        if (BCM_SUCCESS(rv)) {
            for (idx = 0; idx < BCM_ROBO_NUM_POLICER; idx++) {
                if (_policer[unit].policers_used[idx]) {
                    sal_memcpy(pol_cfg, &_policer[unit].policers[idx], 
                        sizeof(bcm_policer_config_t));
                    (*traverse_callback)(unit, idx+1, pol_cfg, cookie);
                }
            }
            POLICER_UNIT_UNLOCK_nr(unit, rv);
        }

        sal_free(pol_cfg);

        return rv;
#endif
    }
#ifdef BCM_FIELD_SUPPORT 
    if (soc_feature(unit, soc_feature_field)) {
        return _bcm_robo_field_policer_traverse(unit, traverse_callback, cookie);
    }
#endif    
    return BCM_E_UNAVAIL;
}

