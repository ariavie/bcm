/*
 * $Id: cmmdebug.h 1.5 Broadcom SDK $
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

#ifndef _SOC_CM_MDEBUG_H
#define _SOC_CM_MDEBUG_H

#include <sal/types.h>
#include <sal/core/libc.h>

/*
 * Each debug option is encoded in the following way:
 *
 *   Bits [7:0]     Encoding word index
 *   Bits [15:8]    Debug level
 *   Bits [31:16]   Debug option mask
 *
 * The current debug configuration is encoded into an array of 32-bit
 * words. Each debug option belongs in a specific word, whereas the
 * debug level is identical across all words.
 */

/* Mask for encoding word index */
#define SOC_CM_MDEBUG_ENC_MASK  0xff

/* Mask for debug level */
#define SOC_CM_MDEBUG_LVL_MASK  (0xff << 8)

/* Mask for debug level */
#define SOC_CM_MDEBUG_OPT_MASK  (0xffff << 16)

/* Encoder for shared debug levels */
#define SOC_CM_MDEBUG_LVL_ENC(l_) (1 << (((l_) & 0x7) + 8))

/* Encoder for per-module debug options */
#define SOC_CM_MDEBUG_OPT_ENC(m_) ((1 << (((m_) & 0xf) + 16)) | ((m_) >> 4))

/* Debug levels for sharing across all debug modules */
#define SOC_CM_DBG_ERR          SOC_CM_MDEBUG_LVL_ENC(0)
#define SOC_CM_DBG_WARN         SOC_CM_MDEBUG_LVL_ENC(1)
#define SOC_CM_DBG_NORMAL       SOC_CM_MDEBUG_LVL_ENC(2)
#define SOC_CM_DBG_VERBOSE      SOC_CM_MDEBUG_LVL_ENC(3)
#define SOC_CM_DBG_VVERBOSE     SOC_CM_MDEBUG_LVL_ENC(4)

#define SOC_CM_DBG_LEVEL_COUNT  5

#define SOC_CM_DBG_NAMES \
    "ERRor",             \
    "WARN",              \
    "NORmal",            \
    "VERbose",           \
    "VVERbose",          \
    ""

/* Number of encoding words - each word provides 16 debug options */
#define SOC_CM_MDEBUG_ENC_MAX   8

/* Number of debug modules supported */
#define SOC_CM_MDEBUG_MOD_MAX   8

typedef struct soc_cm_mdebug_config_s {
    const char *mod_name;
    const char **opt_names;
    int num_opt;
    const char **lvl_names;
    int num_lvl;
    uint32 cur_enc[SOC_CM_MDEBUG_ENC_MAX];
} soc_cm_mdebug_config_t;

/* Default debug level */
#define SOC_CM_DBG_DEFAULT (     \
        SOC_CM_DBG_ERR |         \
        SOC_CM_DBG_WARN |        \
        SOC_CM_DBG_NORMAL        \
        )

/* Helper macros for detailed debugging output */
#define MDEBUG_MSG_LF(str_) "%s:%d(%s): " str_ "\n", __FILE__, __LINE__, FUNCTION_NAME()
#define MDEBUG_MSG_LFU(u_, str_) "%s:%d(%s)[%d]: " str_ "\n", __FILE__, __LINE__, FUNCTION_NAME(), u_
#define MDEBUG_MSG_F(str_) "%s: " str_ "\n", FUNCTION_NAME()
#define MDEBUG_MSG_FU(u_, str_) "%s[%d]: " str_ "\n", FUNCTION_NAME(), u_

/* Call-back function for module/options traversal */
typedef int (*soc_cm_mdebug_cb_t)(soc_cm_mdebug_config_t *dbg_cfg,
                                  const char *opt, int enable, void *context);

/* Debug levels for sharing across all debug modules */
extern const char *soc_cm_mdebug_level_names[];

extern int
soc_cm_mdebug_deinit(void);

extern int
soc_cm_mdebug_init(void);

extern int
soc_cm_mdebug_add(soc_cm_mdebug_config_t *dbg_cfg);

extern int
soc_cm_mdebug_remove(soc_cm_mdebug_config_t *dbg_cfg);

extern int
soc_cm_mdebug_check(soc_cm_mdebug_config_t *dbg_cfg, uint32 enc);

extern int
soc_cm_mdebug_enable_set(soc_cm_mdebug_config_t *dbg_cfg,
                         uint32 enc, int enable);

extern int
soc_cm_mdebug_config_get(const char *mod, soc_cm_mdebug_config_t **dbg_cfg);

extern int
soc_cm_mdebug_encoding_get(soc_cm_mdebug_config_t *dbg_cfg,
                           const char *opt, uint32 *enc);

extern int
soc_cm_mdebug_traverse(soc_cm_mdebug_config_t *dbg_cfg, int enabled,
                       soc_cm_mdebug_cb_t cb_fn, void *context);

#endif  /* !_SOC_CM_MDEBUG_H */
