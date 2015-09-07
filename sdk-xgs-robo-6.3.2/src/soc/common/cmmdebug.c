/*
 * $Id: cmmdebug.c 1.2 Broadcom SDK $
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
 * File:        cm.c
 * Purpose:     Configuration Manager Modular Debug
 *
 */

#include <soc/cmmdebug.h>

static soc_cm_mdebug_config_t *debug_config[SOC_CM_MDEBUG_MOD_MAX];

/* Debug level names are global and may be reused by all modules */
const char *soc_cm_mdebug_level_names[] = { SOC_CM_DBG_NAMES };

/*
 * Function:    soc_cm_mdebug_match
 * Purpose:     Helper function for soc_cm_mdebug_traverse
 *              
 * Parameters:  dbg_cfg - module debug configuration
 *              enc - debug encoding
 *
 * Returns:     TRUE/FALSE
 */
STATIC int
soc_cm_mdebug_match(soc_cm_mdebug_config_t *dbg_cfg, uint32 enc)
{
    int idx = enc & (SOC_CM_MDEBUG_ENC_MAX - 1);

    /* Mask off encoding word index */
    enc &= ~SOC_CM_MDEBUG_ENC_MASK;

    if ((enc & dbg_cfg->cur_enc[idx]) == enc) {
        return TRUE;
    }

    return FALSE;
}

/*
 * Function:    soc_cm_mdebug_deinit
 * Purpose:     Free any allocated resources
 *              
 * Parameters:  None
 *
 * Returns:     Always zero
 */
int
soc_cm_mdebug_deinit(void)
{
    sal_memset(debug_config, 0, sizeof(debug_config));

    return 0;
}

/*
 * Function:    soc_cm_mdebug_init
 * Purpose:     Initialize the module debug manager
 *              
 * Parameters:  None
 *
 * Returns:     Always zero
 */
int
soc_cm_mdebug_init(void)
{
    return soc_cm_mdebug_deinit();
}

/*
 * Function:    soc_cm_mdebug_add
 * Purpose:     Add a module to the module debug manager
 *              
 * Parameters:  dbg_cfg - pointer to module debug configuration
 *
 * Returns:     Always zero
 */
int
soc_cm_mdebug_add(soc_cm_mdebug_config_t *dbg_cfg)
{
    int idx;

    for (idx = 0; idx < COUNTOF(debug_config); idx++) {
        if (debug_config[idx] == NULL) {
            debug_config[idx] = dbg_cfg;
            return idx;
        }
    }

    return -1;
}

/*
 * Function:    soc_cm_mdebug_remove
 * Purpose:     Remove a module gtom the module debug manager
 *              
 * Parameters:  dbg_cfg - pointer to module debug configuration
 *
 * Returns:     Always zero
 */
int
soc_cm_mdebug_remove(soc_cm_mdebug_config_t *dbg_cfg)
{
    int idx;

    for (idx = 0; idx < COUNTOF(debug_config); idx++) {
        if (debug_config[idx] == dbg_cfg) {
            debug_config[idx] = NULL;
            break;
        }
    }

    return 0;
}

/*
 * Function:    soc_cm_mdebug_check
 * Purpose:     Check if a given debug encoding is currently enabled
 *              
 * Parameters:  dbg_cfg - module debug configuration
 *              enc - debug encoding
 *
 * Returns:     TRUE/FALSE
 */
int
soc_cm_mdebug_check(soc_cm_mdebug_config_t *dbg_cfg, uint32 enc)
{
    int idx = enc & (SOC_CM_MDEBUG_ENC_MAX - 1);

    /* Messages without a specific level are consider normal */
    if ((enc & SOC_CM_MDEBUG_LVL_MASK) == 0) {
        enc |= SOC_CM_DBG_NORMAL;
    }

    /* Mask off encoding word index */
    enc &= ~SOC_CM_MDEBUG_ENC_MASK;

    if ((enc & dbg_cfg->cur_enc[idx]) == enc) {
        return TRUE;
    }

    return FALSE;
}

/*
 * Function:    soc_cm_mdebug_enable_set
 * Purpose:     Enable or disable a debug encoding
 *              
 * Parameters:  dbg_cfg - module debug configuration
 *              enc - debug encoding
 *              enable - TRUE/FALSE
 *
 * Returns:     Always zero
 */
int
soc_cm_mdebug_enable_set(soc_cm_mdebug_config_t *dbg_cfg,
                         uint32 enc, int enable)
{
    int idx = enc & (SOC_CM_MDEBUG_ENC_MAX - 1);

    /* Mask off encoding word index */
    enc &= ~SOC_CM_MDEBUG_ENC_MASK;

    /* Update encoding word */
    if (enable) {
        dbg_cfg->cur_enc[idx] |= enc;
    } else {
        dbg_cfg->cur_enc[idx] &= ~enc;
    }

    /* If encoding word contains level then update all words */
    enc &= SOC_CM_MDEBUG_LVL_MASK;
    if (enc) {
        for (idx = 0; idx < COUNTOF(dbg_cfg->cur_enc); idx++) {
            if (enable) {
                dbg_cfg->cur_enc[idx] |= enc;
            } else {
                dbg_cfg->cur_enc[idx] &= ~enc;
            }
        }
    }

    return 0;
}

/*
 * Function:    soc_cm_mdebug_config_get
 * Purpose:     Get debug module configuration for a given module
 *              
 * Parameters:  mod - module name
 *              dbg_cfg (OUT) - pointer to module debug configuration
 *
 * Returns:     Always zero
 */
int
soc_cm_mdebug_config_get(const char *mod, soc_cm_mdebug_config_t **dbg_cfg)
{
    int idx;
    soc_cm_mdebug_config_t *cfg;

    *dbg_cfg = NULL;

    for (idx = 0; idx < COUNTOF(debug_config); idx++) {
        cfg = debug_config[idx];
        if (cfg && sal_strcmp(mod, cfg->mod_name) == 0) {
            *dbg_cfg = cfg;
            break;
        }
    }

    return 0;
}

/*
 * Function:    soc_cm_mdebug_encoding_get
 * Purpose:     Get hex encoding for a module debug option
 *              
 * Parameters:  dbg_cfg - pointer to module debug configuration
 *              opt - name of debug option
 *              enc (OUT) - pointer to hex encoding word
 *
 * Returns:     Zero if encoding was found, otherwise -1.
 */
int
soc_cm_mdebug_encoding_get(soc_cm_mdebug_config_t *dbg_cfg,
                           const char *opt, uint32 *enc)
{
    int idx;

    *enc = 0;

    if (dbg_cfg == NULL || opt == NULL) {
        return -1;
    }

    /* Check configured debug levels */
    for (idx = 0; idx < dbg_cfg->num_lvl; idx++) {
        if (sal_strcmp(opt, dbg_cfg->lvl_names[idx]) == 0) {
            *enc = SOC_CM_MDEBUG_LVL_ENC(idx);
            return 0;
        }
    }

    /* Check configured debug options */
    for (idx = 0; idx < dbg_cfg->num_opt; idx++) {
        if (sal_strcmp(opt, dbg_cfg->opt_names[idx]) == 0) {
            *enc = SOC_CM_MDEBUG_OPT_ENC(idx);
            return 0;
        }
    }

    return -1;
}

/*
 * Function:    soc_cm_mdebug_traverse
 * Purpose:     Invoke call-back function for set of debug options
 *              
 * Parameters:  dbg_cfg - pointer to module debug configuration
 *              enabled - defines set to traverse, -1 means all
 *              cb_fn - call-back function
 *              context - optional user data for call-back function
 *
 * Returns:     Number of call-outs made, or negative if call-back
 *              return a negative value.
 *
 * Notes:       If call-back function returns a negative value then
 *              the traverse operation is terminated immediately.
 *              If dbg_cfg is NULL then the function will traverse
 *              installed debug modules.
 */
int
soc_cm_mdebug_traverse(soc_cm_mdebug_config_t *dbg_cfg, int enabled,
                       soc_cm_mdebug_cb_t cb_fn, void *context)
{
    int idx;
    int chk;
    int rv;
    int cb_cnt;
    uint32 enc;
    soc_cm_mdebug_config_t *cfg;

    cb_cnt = 0;

    /* Loop over installed modules if dbg_cfg is NULL */
    if (dbg_cfg == NULL) {
        for (idx = 0; idx < COUNTOF(debug_config); idx++) {
            cfg = debug_config[idx];
            if (cfg && cb_fn) {
                rv = cb_fn(dbg_cfg, cfg->mod_name, -1, context);
                if (rv < 0) {
                    return rv;
                }
                cb_cnt++;
            }
        }
        return cb_cnt;
    }

    /* Loop over debug levels for this module */
    for (idx = 0; idx < dbg_cfg->num_lvl; idx++) {
        enc = SOC_CM_MDEBUG_LVL_ENC(idx);
        chk = soc_cm_mdebug_match(dbg_cfg, enc);
        if (enabled < 0 || chk == enabled) {
            if (cb_fn) {
                rv = cb_fn(dbg_cfg, dbg_cfg->lvl_names[idx], chk, context);
                if (rv < 0) {
                    return rv;
                }
                cb_cnt++;
            }
        }
    }

    /* Loop over debug options for this module */
    for (idx = 0; idx < dbg_cfg->num_opt; idx++) {
        enc = SOC_CM_MDEBUG_OPT_ENC(idx);
        chk = soc_cm_mdebug_match(dbg_cfg, enc);
        if (enabled < 0 || chk == enabled) {
            if (cb_fn) {
                rv = cb_fn(dbg_cfg, dbg_cfg->opt_names[idx], chk, context);
                if (rv < 0) {
                    return rv;
                }
                cb_cnt++;
            }
        }
    }

    return cb_cnt;
}
