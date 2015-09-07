/*
 * $Id: l2mod.c 1.87.6.1 Broadcom SDK $
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

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/time.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/l2x.h>
#if defined(BCM_TRX_SUPPORT)
#include <soc/triumph.h>
#endif /* BCM_TRX_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT

/****************************************************************************
 *
 * L2MOD Message Registration
 *
 ****************************************************************************/

#define OPER_WRITE      0
#define OPER_PPA        2
#define OPER_DELETE     3

#define REASON_CPU      0
#define REASON_AGE      1
#define REASON_PPA      3

#define REASON_CPU       0
#define REASON_LEARN     1
#define REASON_STAT_MOVE 2
#define REASON_PPA       3

STATIC int
soc_l2_overflow_event_record_set(int unit, int enable) {
#ifdef BCM_TRX_SUPPORT
    uint32 reg;

    if (SOC_IS_TRIDENT(unit)) {
        SOC_IF_ERROR_RETURN(READ_L2_LEARN_CONTROLr(unit, &reg));
        soc_reg_field_set(unit, L2_LEARN_CONTROLr, &reg,
                          L2_RECORD_OVERFLOW_ENABLEf, enable & 1);
        SOC_IF_ERROR_RETURN(WRITE_L2_LEARN_CONTROLr(unit, reg));
        return SOC_E_NONE;
    }
#endif
    return SOC_E_NONE;
}

STATIC int
_soc_l2mod_fifo_enable(int unit, int val)
{
    uint32 reg;
#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        SOC_IF_ERROR_RETURN(READ_AUX_ARB_CONTROLr(unit, &reg));
        /* Does application cares about MEMWR, L2_INSERT, L2_DELETE ? */
        soc_reg_field_set(unit, AUX_ARB_CONTROLr, &reg,
                          L2_MOD_FIFO_ENABLE_AGEf, val);
        soc_reg_field_set(unit, AUX_ARB_CONTROLr, &reg,
                          L2_MOD_FIFO_ENABLE_LEARNf, val);
        soc_reg_field_set(unit, AUX_ARB_CONTROLr, &reg,
                         L2_MOD_FIFO_ENABLE_L2_DELETEf, val);
        if (!soc_feature(unit, soc_feature_l2_bulk_control)) {
            soc_reg_field_set(unit, AUX_ARB_CONTROLr, &reg,
                              L2_MOD_FIFO_ENABLE_PPA_DELETEf, val);
            soc_reg_field_set(unit, AUX_ARB_CONTROLr, &reg,
                              L2_MOD_FIFO_ENABLE_PPA_REPLACEf, val);
        }
        SOC_IF_ERROR_RETURN(WRITE_AUX_ARB_CONTROLr(unit, reg));

        if (soc_feature(unit, soc_feature_l2_bulk_control)) {
            SOC_IF_ERROR_RETURN(READ_L2_BULK_CONTROLr(unit, &reg));
            soc_reg_field_set(unit, L2_BULK_CONTROLr, &reg,
                              L2_MOD_FIFO_RECORDf, 1);
            SOC_IF_ERROR_RETURN(WRITE_L2_BULK_CONTROLr(unit, reg));
        }

        if (SOC_IS_TRIDENT(unit)) {
	        SOC_IF_ERROR_RETURN(soc_l2_overflow_event_record_set(unit, 1));
        }
        
        return SOC_E_NONE;
    }
#endif /* BCM_TRX_SUPPORT */
#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FBX(unit)) {
        SOC_IF_ERROR_RETURN(READ_AUX_ARB_CONTROLr(unit, &reg));
        soc_reg_field_set(unit, AUX_ARB_CONTROLr, &reg, L2_MOD_FIFO_ENABLEf, val);
        SOC_IF_ERROR_RETURN(WRITE_AUX_ARB_CONTROLr(unit, reg));
    }
#endif
    return SOC_E_NONE;
}

STATIC int
_soc_l2mod_fifo_lock(int unit, int val)
{
    uint32 reg;
#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FBX(unit)) {
        SOC_IF_ERROR_RETURN(READ_AUX_ARB_CONTROLr(unit, &reg));
        soc_reg_field_set(unit, AUX_ARB_CONTROLr, &reg, L2_MOD_FIFO_LOCKf, val);
        SOC_IF_ERROR_RETURN(WRITE_AUX_ARB_CONTROLr(unit, reg));
    }
#endif
    return SOC_E_NONE;
}

STATIC int
_soc_l2mod_fifo_get_count(int unit, int * cnt)
{
    uint32 reg;
#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FBX(unit)) {
        SOC_IF_ERROR_RETURN(READ_L2_MOD_FIFO_CNTr(unit, &reg));
        *cnt = soc_reg_field_get(unit, L2_MOD_FIFO_CNTr, reg, NUM_OF_ENTRIESf);
    }
#endif
    return SOC_E_NONE;
}


#ifdef BCM_KATANA_SUPPORT
 void
_soc_kt_l2mod_fifo_process(int unit, uint32 flags, l2_mod_fifo_entry_t *entry)
{
    int operation;
    l2x_entry_t l2x_entry, old_l2x_entry;
    uint32 fval[SOC_MAX_MEM_FIELD_WORDS];
    int len;

    operation = soc_L2_MOD_FIFOm_field32_get(unit, entry, OPERATIONf);
    soc_L2_MOD_FIFOm_field_get(unit, entry, L2_ENTRY_DATAf,
                               l2x_entry.entry_data);
    if (soc_mem_field32_get(unit, L2_MOD_FIFOm, entry, STATION_MOVEf)) {
        sal_memcpy(&old_l2x_entry, &l2x_entry, sizeof(l2x_entry));
        soc_mem_field_get(unit, L2_MOD_FIFOm, (uint32 *)entry,
                          REPLACED_ASSOC_DATAf, fval);
        len = soc_mem_field_length(unit, L2Xm, ASSOCIATED_DATAf);
        if (len & 0x1f) {
            fval[len / 32] &= (1 << (len & 0x1f)) - 1;
        }
        soc_mem_field_set(unit, L2Xm, (uint32 *)&old_l2x_entry,
                          ASSOCIATED_DATAf, fval);
    }

    switch (operation) {
    case 0: /* L2_DELETE */
    case 1: /* PPA DELETE */
    case 2: /* AGE */
    case 9: /* L2_BULK_DELETE */
    case 11: /* L2_BULK_AGE */
        soc_l2x_callback(unit, 0, &l2x_entry, NULL);
        break;
    case 10: /* L2_BULK_REPLACE */
    case  4: /* PPA REPLACE */
        soc_l2x_callback(unit, 0, &old_l2x_entry, &l2x_entry);
        break;
    case 3: /* L2_INSERT */
    case 5: /* LEARN */
        soc_l2x_callback(unit, 0, NULL, &l2x_entry);
        break;
    case 6: /* MEMWR, S/W mark for the end of matched traverse */
    case 7: /* OVERFLOW */
    case 8: /* L2_BULK_NO_ACTION */
    case 12: /* MAC_LIMIT_DELETE_FOR_INSERT */
    case 13: /* MAC_LIMIT_DELETE_FOR_PPA_REPLACE */
    case 14: /* MAC_LIMIT_DELETE_FOR_LEARN */
    default:
        break;
    }

    if (soc_cm_debug_check(DK_DMA | DK_VERBOSE)) {
        soc_mem_entry_dump(unit, L2_MOD_FIFOm, entry);
        soc_cm_print("\n");
    }
}
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
void
_soc_td2_l2mod_fifo_process(int unit, uint32 flags, l2_mod_fifo_entry_t *entry)
{
    int operation;
    l2x_entry_t l2x_entry, old_l2x_entry;
    uint32 fval[SOC_MAX_MEM_FIELD_WORDS];
    int len;

    operation = soc_L2_MOD_FIFOm_field32_get(unit, entry, OPERATIONf);
    soc_L2_MOD_FIFOm_field_get(unit, entry, L2_ENTRYf, l2x_entry.entry_data);
    if (soc_mem_field32_get(unit, L2_MOD_FIFOm, entry, STATION_MOVEf)) {
        sal_memcpy(&old_l2x_entry, &l2x_entry, sizeof(l2x_entry));
        soc_mem_field_get(unit, L2_MOD_FIFOm, (uint32 *)entry,
                          REPLACED_ASSOC_DATAf, fval);
        len = soc_mem_field_length(unit, L2Xm, ASSOCIATED_DATAf);
        if (len & 0x1f) {
            fval[len / 32] &= (1 << (len & 0x1f)) - 1;
        }
        soc_mem_field_set(unit, L2Xm, (uint32 *)&old_l2x_entry,
                          ASSOCIATED_DATAf, fval);
    }

    switch (operation) {
    case 0: /* L2_DELETE */
    case 4: /* AGE */
    case 6: /* L2_BULK_DELETE */
    case 8: /* L2_BULK_AGE */
        soc_l2x_callback(unit, 0, &l2x_entry, NULL);
        break;
    case 7: /* L2_BULK_REPLACE */
        soc_l2x_callback(unit, 0, &old_l2x_entry, &l2x_entry);
        break;
    case 1: /* L2_INSERT */
    case 2: /* LEARN */

        if (soc_mem_field32_get(unit, L2_MOD_FIFOm, entry, STATION_MOVEf)) {
            soc_l2x_callback(unit, 0, &old_l2x_entry, &l2x_entry);
        } else {
        soc_l2x_callback(unit, 0, NULL, &l2x_entry);
        }

        break;
    case 3: /* MEMWR, S/W mark for the end of matched traverse */
        soc_l2x_callback(unit, SOC_L2X_ENTRY_DUMMY, NULL, NULL);
        break;
    case 5: /* L2_BULK_NO_ACTION */
        soc_l2x_callback(unit, SOC_L2X_ENTRY_NO_ACTION, NULL, &l2x_entry);
        break;
    default:
        break;
    }

    if (soc_cm_debug_check(DK_DMA | DK_VERBOSE)) {
        soc_mem_entry_dump(unit, L2_MOD_FIFOm, entry);
        soc_cm_print("\n");
    }
}
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
STATIC void
_soc_td_l2mod_fifo_process(int unit, uint32 flags, l2_mod_fifo_entry_t *entry,
			        uint32 *poverflow_event_counter, uint32 overflow_event_threshhold)
{
    int operation;
    l2x_entry_t l2x_entry, old_l2x_entry;
    uint32 fval[SOC_MAX_MEM_FIELD_WORDS];
    int len;

    operation = soc_L2_MOD_FIFOm_field32_get(unit, entry, OPERATIONf);
    soc_L2_MOD_FIFOm_field_get(unit, entry, L2_ENTRY_DATAf,
                               l2x_entry.entry_data);
    if (soc_mem_field32_get(unit, L2_MOD_FIFOm, entry, STATION_MOVEf)) {
        sal_memcpy(&old_l2x_entry, &l2x_entry, sizeof(l2x_entry));
        soc_mem_field_get(unit, L2_MOD_FIFOm, (uint32 *)entry,
                          REPLACED_ASSOC_DATAf, fval);
        len = soc_mem_field_length(unit, L2Xm, ASSOCIATED_DATAf);
        if (len & 0x1f) {
            fval[len / 32] &= (1 << (len & 0x1f)) - 1;
        }
        soc_mem_field_set(unit, L2Xm, (uint32 *)&old_l2x_entry,
                          ASSOCIATED_DATAf, fval);
    }

    switch (operation) {
    case 0: /* L2_DELETE */
    case 1: /* AGE */
    case 7: /* L2_BULK_DELETE */
    case 9: /* L2_BULK_AGE */
        soc_l2x_callback(unit, 0, &l2x_entry, NULL);
        break;
    case 8: /* L2_BULK_REPLACE */
        soc_l2x_callback(unit, 0, &old_l2x_entry, &l2x_entry);
        break;
    case 2: /* L2_INSERT */
    case 3: /* LEARN */
        soc_l2x_callback(unit, 0, NULL, &l2x_entry);
        break;
    case 4: /* MEMWR, S/W mark for the end of matched traverse */
        soc_l2x_callback(unit, SOC_L2X_ENTRY_DUMMY, NULL, NULL);
        break;
    case 5: /* OVERFLOW */
        if (SOC_IS_TRIDENT(unit)) {        
            (*poverflow_event_counter)++;
            if (*poverflow_event_counter < overflow_event_threshhold) {
    	        soc_l2x_callback(unit, SOC_L2X_ENTRY_OVERFLOW, NULL, &l2x_entry);
            } else if (*poverflow_event_counter == overflow_event_threshhold) {
    	        (void)soc_l2_overflow_event_record_set(unit, 0);
            }
        }
      break;
    case 6: /* L2_BULK_NO_ACTION */
        soc_l2x_callback(unit, SOC_L2X_ENTRY_NO_ACTION, NULL, &l2x_entry);
        break;
    default:
        break;
    }

    if (soc_cm_debug_check(DK_DMA | DK_VERBOSE)) {
        soc_mem_entry_dump(unit, L2_MOD_FIFOm, entry);
        soc_cm_print("\n");
    }
}
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_TRX_SUPPORT)
STATIC void
_soc_tr_l2mod_fifo_process(int unit, uint32 flags, l2_mod_fifo_entry_t *entry)
{
    int operation;
    l2x_entry_t l2x_entry, old_l2x_entry;
    uint32 fval[SOC_MAX_MEM_FIELD_WORDS];
    int len;
#ifdef BCM_TRIUMPH_SUPPORT
    int rv, rv1;
    int index;
    ext_l2_entry_entry_t ext_l2_entry, old_ext_l2_entry;
#endif /* BCM_TRIUMPH_SUPPORT */

    operation = soc_L2_MOD_FIFOm_field32_get(unit, entry, OPERATIONf);
    soc_L2_MOD_FIFOm_field_get(unit, entry, L2_ENTRY_DATAf,
                               l2x_entry.entry_data);
    if (soc_mem_field32_get(unit, L2_MOD_FIFOm, entry, STATION_MOVEf)) {
        sal_memcpy(&old_l2x_entry, &l2x_entry, sizeof(l2x_entry));
        soc_mem_field_get(unit, L2_MOD_FIFOm, (uint32 *)entry,
                          REPLACED_ASSOC_DATAf, fval);
        len = soc_mem_field_length(unit, L2Xm, ASSOCIATED_DATAf);
        if (len & 0x1f) {
            fval[len / 32] &= (1 << (len & 0x1f)) - 1;
        }
        soc_mem_field_set(unit, L2Xm, (uint32 *)&old_l2x_entry,
                          ASSOCIATED_DATAf, fval);
    }

    switch (operation) {
    case 1: /* PPA_DELETE */
    case 2: /* AGE */
        soc_l2x_callback(unit, 0, &l2x_entry, NULL);
        break;
    case 4: /* PPA_REPLACE */
        soc_l2x_callback(unit, 0, &old_l2x_entry, &l2x_entry);
        break;
    case 5: /* LEARN */
#ifdef BCM_TRIUMPH_SUPPORT
        if (soc_mem_is_valid(unit, EXT_L2_ENTRYm) &&
            soc_mem_index_count(unit, EXT_L2_ENTRYm) > 0 &&
            !soc_mem_field32_get(unit, L2Xm, &l2x_entry, PENDINGf)) {
            soc_triumph_l2x_to_ext_l2(unit, &l2x_entry, &ext_l2_entry);

            soc_mem_lock(unit, L2Xm);
            soc_mem_lock(unit, EXT_L2_ENTRYm);
            rv = soc_mem_generic_lookup(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY, 0,
                                        &ext_l2_entry, &old_ext_l2_entry,
                                        NULL);
            if (SOC_SUCCESS(rv)) {
                if (soc_mem_field32_get(unit, EXT_L2_ENTRYm,
                                        &old_ext_l2_entry, STATIC_BITf)) {
                    /*
                     * If an static entry for the same lookup key has been
                     * inserted to external table before we have chance to
                     * process the learn event.
                     * Ignore the learn event, no call back.
                     */
                    soc_mem_unlock(unit, L2Xm);
                    soc_mem_unlock(unit, EXT_L2_ENTRYm);
                    break;
                }
            } 
            rv = soc_mem_generic_insert(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY, 0,
                                        &ext_l2_entry, &old_ext_l2_entry,
                                        NULL);
            if (SOC_SUCCESS(rv) || rv == SOC_E_EXISTS) {
                int is_static;
                rv1 = soc_mem_generic_lookup(unit, L2Xm, MEM_BLOCK_ANY, 0,
                                             &l2x_entry, &old_l2x_entry,
                                             &index);
                is_static = soc_mem_field32_get(unit, L2Xm, &old_l2x_entry, 
                                                STATIC_BITf);
                if ((SOC_SUCCESS(rv1) && is_static) || 
                    ((rv1 == SOC_E_NOT_FOUND) && SOC_SUCCESS(rv))) {
                    /*
                     * If either an static entry for the same lookup key has
                     * been inserted to internal table or the entry has been
                     * delete from internal table before we have chance to
                     * process the mod fifo learn event.
                     * Ignore the learn event (undo insert), no call back.
                     */
                    soc_mem_generic_delete(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY,
                                           0, &ext_l2_entry, NULL, NULL);
                    soc_mem_unlock(unit, L2Xm);
                    soc_mem_unlock(unit, EXT_L2_ENTRYm);
                    break;
                }
                if (SOC_SUCCESS(rv1)) {
                    (void)soc_mem_field32_modify(unit, L2Xm, index,
                                                 LIMIT_COUNTEDf, 0);
                    (void)soc_mem_generic_delete(unit, L2Xm, MEM_BLOCK_ANY,
                                                 0, &l2x_entry, NULL, NULL);
                    if (rv == SOC_E_EXISTS ) {
                        /* Update counts as its a move (while using ESM) */
                        soc_triumph_learn_count_update(unit, &old_ext_l2_entry,
                                                       FALSE, -1);
                    }
                }
                soc_mem_unlock(unit, L2Xm);
                soc_mem_unlock(unit, EXT_L2_ENTRYm);

                if (rv == SOC_E_EXISTS) {
                    /* Do extra delete callback if the entry exists in ext
                     * table but has "deleted" mark. */
                    if (!soc_mem_field32_get(unit, EXT_L2_ENTRYm,
                                             &old_ext_l2_entry, VALIDf)) {
                        soc_triumph_ext_l2_to_l2x(unit, &old_ext_l2_entry,
                                                  &old_l2x_entry);
                        soc_l2x_callback(unit, 0, &old_l2x_entry, NULL);
                    }
                }
            } else {
                soc_mem_unlock(unit, L2Xm);
                soc_mem_unlock(unit, EXT_L2_ENTRYm);
            }
        }
#endif /* BCM_TRIUMPH_SUPPORT */
        if (soc_feature(unit, soc_feature_ppa_bypass)) {
            if (SOC_CONTROL(unit)->l2x_ppa_bypass == FALSE &&
                soc_mem_field32_get(unit, L2Xm, &l2x_entry, KEY_TYPEf) !=
                TR_L2_HASH_KEY_TYPE_BRIDGE) {
                SOC_CONTROL(unit)->l2x_ppa_bypass = TRUE;
            }
        }
        soc_l2x_callback(unit, 0, NULL, &l2x_entry);
        break;
    case 0: /* L2_DELETE */
            soc_l2x_callback(unit, 0, &l2x_entry, NULL);
            break;
    case 3: /* L2_INSERT */
    case 6: /* MEMWR */
    case 7: /* OVERFLOW */
    case 11: /* MAC_LIMIT_DELETE_FOR_INSERT */
    case 12: /* MAC_LIMIT_DELETE_FOR_PPA_REPLACE */
    case 13: /* MAC_LIMIT_DELETE_FOR_LEARN */
    default:
        break;
    }

    if (soc_cm_debug_check(DK_DMA | DK_VERBOSE)) {
        soc_mem_entry_dump(unit, L2_MOD_FIFOm, entry);
        soc_cm_print("\n");
    }
}

#ifdef BCM_TRIUMPH_SUPPORT
STATIC void
_soc_tr_ext_l2mod_fifo_process(int unit, uint32 flags,
                               ext_l2_mod_fifo_entry_t *entry)
{
    int rv, typ;
    l2x_entry_t l2x_entry, old_l2x_entry;
    ext_l2_entry_entry_t ext_l2_entry, old_ext_l2_entry;
    ext_l2_entry_tcam_entry_t tcam_entry;
    uint32 rval, index;
    sal_mac_addr_t mac;
    static uint32 repl_t = 0, repl_dest = 0;

    soc_mem_field_get(unit, EXT_L2_MOD_FIFOm, (uint32 *)entry, WR_DATAf,
                      (uint32 *)&ext_l2_entry);
    index = soc_mem_field32_get(unit, EXT_L2_MOD_FIFOm, entry, ENTRY_ADRf);

    /*
     * ESM_L2_AGE_CTL.EN_RD_TCAM and ESM_PER_PORTREPL_CONTROL.EN_RD_TCAM
     * are required to be set to same value (even if aging is disable)
     */
    if (SOC_FAILURE(READ_ESM_L2_AGE_CTLr(unit, &rval))) {
        return;
    }
    if (!soc_reg_field_get(unit, ESM_L2_AGE_CTLr, rval, EN_RD_TCAMf)) {
        if (SOC_FAILURE(soc_mem_read(unit, EXT_L2_ENTRY_TCAMm,
                                     MEM_BLOCK_ANY, index, &tcam_entry))) {
            return;
        }
        soc_mem_mac_addr_get(unit, EXT_L2_ENTRY_TCAMm, &tcam_entry, MAC_ADDRf,
                             mac);
        soc_mem_mac_addr_set(unit, EXT_L2_ENTRYm, &ext_l2_entry, MAC_ADDRf,
                             mac);
    }

    typ = soc_mem_field32_get(unit, EXT_L2_MOD_FIFOm, entry, TYPf);
    switch (typ) {
    case 3: /* special PPA REPL marker by software */
        repl_t = soc_mem_field32_get(unit, EXT_L2_ENTRYm, &ext_l2_entry, Tf);
        repl_dest = soc_mem_field32_get(unit, EXT_L2_ENTRYm, &ext_l2_entry,
                                        DESTINATIONf);
        break;
    case 4: /* REPLACED */
        sal_memcpy(&old_ext_l2_entry, &ext_l2_entry,
                   soc_mem_entry_words(unit, EXT_L2_ENTRYm) * 4);
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, &ext_l2_entry, Tf, repl_t);
        soc_mem_field32_set(unit, EXT_L2_ENTRYm, &ext_l2_entry, DESTINATIONf,
                            repl_dest);
        soc_mem_lock(unit, EXT_L2_ENTRYm);
        soc_triumph_learn_count_update(unit, &ext_l2_entry, FALSE, 1);
        
        soc_triumph_ext_l2_entry_update(unit, index, &ext_l2_entry);
        soc_triumph_learn_count_update(unit, &old_ext_l2_entry, FALSE, -1);
        soc_mem_unlock(unit, EXT_L2_ENTRYm);
        soc_triumph_ext_l2_to_l2x(unit, &old_ext_l2_entry, &old_l2x_entry);
        soc_triumph_ext_l2_to_l2x(unit, &ext_l2_entry, &l2x_entry);
        soc_l2x_callback(unit, 0, &old_l2x_entry, &l2x_entry);
        break;
    case 5: /* DELETED */
    case 6: /* ENTRY_IS_OLD */
        soc_mem_lock(unit, EXT_L2_ENTRYm);
        soc_triumph_ext_l2_entry_update(unit, index, NULL);
        soc_triumph_learn_count_update(unit, &ext_l2_entry, TRUE, -1);
        soc_mem_unlock(unit, EXT_L2_ENTRYm);
        soc_triumph_ext_l2_to_l2x(unit, &ext_l2_entry, &l2x_entry);
        soc_l2x_callback(unit, 0, &l2x_entry, NULL);
        break;
    case 7: /* CLEARED_VALID */
        rv = soc_mem_generic_lookup(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY, 0,
                                    &ext_l2_entry, &old_ext_l2_entry, 0);
        if (rv != SOC_E_NONE) {
            return;
        }

        /* delete only if the VALID bit is cleared */
        if (soc_mem_field32_get(unit, EXT_L2_ENTRYm, &old_ext_l2_entry,
            VALIDf)) {
            return;
        }

        soc_mem_generic_delete(unit, EXT_L2_ENTRYm, MEM_BLOCK_ANY, 0,
                               &ext_l2_entry, NULL, 0);
        soc_triumph_learn_count_update(unit, &ext_l2_entry, TRUE, -1);
        soc_triumph_ext_l2_to_l2x(unit, &ext_l2_entry, &l2x_entry);
        soc_l2x_callback(unit, 0, &l2x_entry, NULL);
        break;
    default:
        break;
    }
}
#endif /* BCM_TRIUMPH_SUPPORT */
#endif /* BCM_TRX_SUPPORT */

STATIC void
_soc_l2mod_fifo_process(int unit, uint32 flags, l2_mod_fifo_entry_t * entry)
{
    int op;
    l2x_entry_t wr_data;

    op     = soc_L2_MOD_FIFOm_field32_get(unit, entry, OPERf);

    soc_L2_MOD_FIFOm_field_get(unit, entry, WR_DATAf, wr_data.entry_data);

    /*
     * No need to translate. A callback expects an l2x_entry_t, and
     * does the translation itselt via mbcm.
     */

    /*
     * The l2mod_fifo_thread gets _changes_, and doesn't need the shadow
     * table; therefore no processing happens here. Just call the callback.
     */
    switch (op) {
        case OPER_DELETE:
            soc_l2x_callback(unit, 0, &wr_data, NULL);
            break;
        case OPER_PPA:
            soc_l2x_callback(unit, 0, &wr_data, &wr_data);
            break;
        case OPER_WRITE:
            soc_l2x_callback(unit, 0, NULL, &wr_data);
            break;
        default:
            soc_cm_debug(DK_ERR, "soc_l2mod_fifo_thread: invalid operation\n");
            break;
    }
}

STATIC void
_soc_l2mod_thread(void * unit_vp)
{
    int unit = PTR_TO_INT(unit_vp);
    soc_control_t * soc = SOC_CONTROL(unit);
    uint32 *fifo;
    int mfifo_words = soc_mem_entry_words(unit, L2_MOD_FIFOm);
    int count;
    uint32 i;
    int interval;
    int rv;
    uint32 index_min, index_max;

    count = soc_mem_index_count(unit, L2_MOD_FIFOm);
    index_min = soc_mem_index_min(unit, L2_MOD_FIFOm);
    index_max = soc_mem_index_max(unit, L2_MOD_FIFOm);
    fifo = soc_cm_salloc(unit,
                         WORDS2BYTES(mfifo_words) * count,
                         "L2_MOD_FIFOm");
    if (fifo == NULL) {
        soc->l2x_pid = SAL_THREAD_ERROR;
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_L2MOD, __LINE__, 
                           SOC_E_MEMORY);
        sal_thread_exit(0);
        return;
    }
    while ((interval = soc->l2x_interval) != 0) {
        /*
         * Post the semaphore -- it will be signalled by the ISR when
         * new data appear in MOD_FIFO.
         */
        sal_sem_take(soc->arl_notify, interval);

        /* Lock the fifo */
        _soc_l2mod_fifo_lock(unit, 1);

        /* Get the number of entries */
        _soc_l2mod_fifo_get_count(unit, &count);

        if (count == 0) {
            _soc_l2mod_fifo_lock(unit, 0);
            continue;
        }

        /* Read in the fifo contents */
        if ((rv = soc_mem_read_range(unit, L2_MOD_FIFOm, MEM_BLOCK_ANY,
                        index_min, index_max, fifo)) < 0) {
            soc_cm_debug(DK_ERR,
                    "soc_l2mod_fifo_thread: DMA failed: %s\n",
                    soc_errmsg(rv));
            soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                               SOC_SWITCH_EVENT_THREAD_L2MOD, __LINE__, rv);
                goto cleanup_exit;
            }
        /* Unlock (and clear) the fifo */
        _soc_l2mod_fifo_lock(unit, 0);

        /* Do the callback for each entry */
        for (i = index_min; i < (index_min + count); i++) {
            if (!soc->l2x_interval) {
                goto cleanup_exit;
            }
            _soc_l2mod_fifo_process(
                            unit,
                            soc->l2x_flags,
                            (l2_mod_fifo_entry_t *)(fifo + i * mfifo_words));
        }
    }
cleanup_exit:
    soc_cm_sfree(unit, fifo);
    soc->l2x_pid = SAL_THREAD_ERROR;
    sal_thread_exit(0);
}

extern uint32 soc_mem_fifo_delay_value;

#ifdef BCM_TRX_SUPPORT
STATIC void
_soc_l2mod_dma_thread(void *unit_vp)
{
    int unit = PTR_TO_INT(unit_vp);
    soc_control_t *soc = SOC_CONTROL(unit);
    int rv, entries_per_buf, adv_threshold, interval, count, i, non_empty;
    int chan[2], entry_words[2];
    void *host_buf[2], *host_entry;
    soc_mem_t mem[2];
    uint32 intr_mask;
#ifdef BCM_CMICM_SUPPORT
    uint32 rval = 0;
    uint8 overflow = 0, timeout = 0;
    int cmc = SOC_PCI_CMC(unit);
#endif /* BCM_CMICM_SUPPORT */
#ifdef BCM_TRIDENT_SUPPORT
    uint32 overflow_event_threshhold;
    uint32 overflow_event_counter;
    sal_usecs_t cur_time, clear_time, time_diff;
#endif

    chan[1] = -1;
    mem[1] = INVALIDm;
    entry_words[1] = 0;
    host_buf[1] = NULL;

    entries_per_buf = soc_property_get(unit, spn_L2XMSG_HOSTBUF_SIZE, 1024);
    adv_threshold = entries_per_buf / 2;

    chan[0] = SOC_MEM_FIFO_DMA_CHANNEL_1;
    mem[0] = L2_MOD_FIFOm;
    entry_words[0] = soc_mem_entry_words(unit, mem[0]);
    intr_mask = IRQ_FIFO_CH1_DMA;
    host_buf[0] =
        soc_cm_salloc(unit, entries_per_buf * entry_words[0] * sizeof(uint32),
                      "L2_MOD DMA Buffer");
    if (host_buf[0] == NULL) {
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_L2MOD_DMA, __LINE__, 
                           SOC_E_MEMORY);
        goto cleanup_exit;
    }

#if defined (BCM_TRIUMPH_SUPPORT)
    if (soc_feature(unit, soc_feature_esm_support)) {
        chan[1] = SOC_MEM_FIFO_DMA_CHANNEL_2;
        mem[1] = EXT_L2_MOD_FIFOm;
        entry_words[1] = soc_mem_entry_words(unit, mem[1]);
        intr_mask |= IRQ_FIFO_CH2_DMA;
        host_buf[1] =
            soc_cm_salloc(unit, entries_per_buf * entry_words[1] *
                          sizeof(uint32), "EXT_L2_MOD DMA Buffer");
        if (host_buf[1] == NULL) {
            soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                               SOC_SWITCH_EVENT_THREAD_L2MOD_DMA, __LINE__, 
                               SOC_E_MEMORY);
            goto cleanup_exit;
        }
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    rv = soc_mem_fifo_dma_start(unit, chan[0], mem[0], MEM_BLOCK_ANY,
                                entries_per_buf, host_buf[0]);
    if (SOC_FAILURE(rv)) {
        soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                           SOC_SWITCH_EVENT_THREAD_L2MOD_DMA, __LINE__, rv);
        goto cleanup_exit;
    }

#if defined (BCM_TRIUMPH_SUPPORT)
    if (mem[1] != INVALIDm) {
        rv = soc_reg_field32_modify(unit, ESM_L2_AGE_CTLr, REG_PORT_ANY,
                                   CPU_NOTIFYf, 1); 
        if (SOC_FAILURE(rv)) {
            soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                               SOC_SWITCH_EVENT_THREAD_L2MOD_DMA, __LINE__, rv);
            goto cleanup_exit;
        }
        rv = soc_reg_field32_modify(unit, ESM_PER_PORT_REPL_CONTROLr,
                                   REG_PORT_ANY, CPU_NOTIFYf, 1); 
        if (SOC_FAILURE(rv)) {
            soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                               SOC_SWITCH_EVENT_THREAD_L2MOD_DMA, __LINE__, rv);
            goto cleanup_exit;
        }
        rv = soc_reg_field32_modify(unit, ESM_CTLr, REG_PORT_ANY,
                                   L2MODFIFO_PUSH_ENf, 1); 
        if (SOC_FAILURE(rv)) {
            soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                               SOC_SWITCH_EVENT_THREAD_L2MOD_DMA, __LINE__, rv);
            goto cleanup_exit;
        }
        rv = soc_mem_fifo_dma_start(unit, chan[1], mem[1], MEM_BLOCK_ANY,
                                   entries_per_buf, host_buf[1]); 
        if (SOC_FAILURE(rv)) {
            soc_event_generate(unit, SOC_SWITCH_EVENT_THREAD_ERROR, 
                               SOC_SWITCH_EVENT_THREAD_L2MOD_DMA, __LINE__, rv);
            goto cleanup_exit;
        }
    }
#endif /* BCM_TRIUMPH_SUPPORT */
    soc_mem_fifo_delay_value =
        soc_property_get(unit, spn_FIFO_DELAY_VALUE, (15 * MILLISECOND_USEC));

#ifdef BCM_TRIDENT_SUPPORT
        overflow_event_threshhold = soc_property_get(unit, "fifo_overflow_threshhold", 100);
        overflow_event_counter = 0;
        clear_time = sal_time_usecs();    
#endif

    while ((interval = soc->l2x_interval)) {
        if (soc->l2modDmaIntrEnb) {
#ifdef BCM_CMICM_SUPPORT
            if (SOC_IS_KATANA(unit)) { 
                soc_cmicm_intr0_enable(unit, 
                                IRQ_CMCx_FIFO_CH_DMA(SOC_MEM_FIFO_DMA_CHANNEL_1));
            } else 
#endif /* BCM_CMICM_SUPPORT */
            {
                soc_intr_enable(unit, intr_mask);
            }
            if (sal_sem_take(soc->arl_notify, interval) < 0) {
                soc_cm_debug(DK_ARL | DK_VERBOSE,
                             "%s polling timeout soc_mem_fifo_delay_value=%d\n", 
                             soc->l2x_name, soc_mem_fifo_delay_value);

#ifdef BCM_TRIDENT_SUPPORT
                if (SOC_IS_TRIDENT(unit)) {
               	    overflow_event_counter = 0;
               	    (void)soc_l2_overflow_event_record_set(unit, 1);
                }
#endif                    
            } else {
                soc_cm_debug(DK_ARL | DK_VERBOSE,
                             "%s woken up soc_mem_fifo_delay_value=%d\n", 
                             soc->l2x_name, soc_mem_fifo_delay_value);
#ifdef BCM_CMICM_SUPPORT
                if (SOC_IS_KATANA(unit)) { 
                     /* check for timeout & overflow */
                    rval = soc_pci_read(unit, 
                              CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_OFFSET(cmc, 
                                              SOC_MEM_FIFO_DMA_CHANNEL_1));          
                    overflow = soc_reg_field_get(unit, 
                                     CMIC_CMC0_FIFO_CH1_RD_DMA_STATr, rval,
                                             FIFO_CH1_DMA_HOSTMEM_TIMEOUTf); 
                    if (overflow) {                         
                        timeout = soc_reg_field_get(unit, 
                                        CMIC_CMC0_FIFO_CH1_RD_DMA_STATr,
                                        rval, FIFO_CH1_DMA_HOSTMEM_OVERFLOWf);
                        overflow |= timeout;
                    }
                }
#endif /* BCM_CMICM_SUPPORT */
#ifdef BCM_TRIDENT_SUPPORT
                if (SOC_IS_TRIDENT(unit)) {
            		cur_time = sal_time_usecs();
            		time_diff = SAL_USECS_SUB( cur_time, clear_time );
            		if (time_diff > interval) {
            		    overflow_event_counter = 0;
            		    (void)soc_l2_overflow_event_record_set(unit, 1);
            		    clear_time = sal_time_usecs();
                    }
                }
#endif                 
            }
        } else {
            sal_usleep(interval);
#ifdef BCM_TRIDENT_SUPPORT
            if (SOC_IS_TRIDENT(unit)) {            
    	        overflow_event_counter = 0;
        	    (void)soc_l2_overflow_event_record_set(unit, 1);
        	    clear_time = sal_time_usecs();
            }
#endif              
        }

        do {
            non_empty = FALSE;
            rv = soc_mem_fifo_dma_get_read_ptr(unit, chan[0], &host_entry,
                                               &count);
            if (SOC_SUCCESS(rv)) {
                non_empty = TRUE;
                if (count > adv_threshold) {
                    count = adv_threshold;
                }
                for (i = 0; i < count; i++) {
                    if(!soc->l2x_interval) {
                        goto cleanup_exit;
                    }
#ifdef BCM_TRIDENT_SUPPORT
                    if (soc_feature(unit, soc_feature_l2_bulk_control)) {
#ifdef BCM_KATANA_SUPPORT
                         if (SOC_IS_KATANA(unit)) {
                             _soc_kt_l2mod_fifo_process(unit, soc->l2x_flags,
                                                                host_entry);

                         } else
#endif /* BCM_KATANA_SUPPORT */
                         {
                             _soc_td_l2mod_fifo_process(unit, soc->l2x_flags,
                                                   host_entry,
                                                   &overflow_event_counter, 
                                                   overflow_event_threshhold);
                         }
                    } else
#endif /* BCM_TRIDENT_SUPPORT */
                    {
                        _soc_tr_l2mod_fifo_process(unit, soc->l2x_flags,
                                                   host_entry);
                    }
                    host_entry = (uint32 *)host_entry + entry_words[0];
                    /*
                     * PPA may wait for available space in mod_fifo, lower the
                     * threshold when ppa is running, therefore read point can
                     * be updated sooner.
                     */
                    if (SOC_CONTROL(unit)->l2x_ppa_in_progress && i >= 63) {
                        i++;
                        break;
                    }
                }
                (void)soc_mem_fifo_dma_advance_read_ptr(unit, chan[0], i);
            }
#if defined(BCM_TRIUMPH_SUPPORT)
            if (mem[1] == INVALIDm) {
                if (!SOC_CONTROL(unit)->l2x_ppa_in_progress) {
                    sal_thread_yield();
                }
                continue;
            }

            rv = soc_mem_fifo_dma_get_read_ptr(unit, chan[1], &host_entry,
                                               &count);
            if (SOC_SUCCESS(rv)) {
                non_empty = TRUE;
                if (count > adv_threshold) {
                    count = adv_threshold;
                }
                for (i = 0; i < count; i++) {
                    if(!soc->l2x_interval) {
                        goto cleanup_exit;
                    }
                    _soc_tr_ext_l2mod_fifo_process(unit, soc->l2x_flags,
                                                   host_entry);
                    host_entry = (uint32 *)host_entry + entry_words[1];
                    /*
                     * PPA may wait for available space in mod_fifo, lower the
                     * threshold when ppa is running, therefore read point can
                     * be updated sooner.
                     */
                    if (SOC_CONTROL(unit)->l2x_ppa_in_progress && i >= 63) {
                        i++;
                        break;
                    }
                }
                (void)soc_mem_fifo_dma_advance_read_ptr(unit, chan[1], i);
            }
#endif /* BCM_TRIUMPH_SUPPORT */

            if (!SOC_CONTROL(unit)->l2x_ppa_in_progress) {
                sal_thread_yield();
            }
        } while (non_empty); 
        /* clearing of the FIFO_CH_DMA_INT interrupt by resetring 
           overflow & timeout status in FIFO_CHx_RD_DMA_STAT_CLR reg */
#ifdef BCM_CMICM_SUPPORT
            if (SOC_IS_KATANA(unit)) {
                if (overflow) {
                    rval = 0;
                    soc_reg_field_set(unit, 
                                 CMIC_CMC0_FIFO_CH1_RD_DMA_STAT_CLRr, &rval,
                                          FIFO_CH1_DMA_HOSTMEM_OVERFLOWf, 1);
                    soc_reg_field_set(unit, 
                                 CMIC_CMC0_FIFO_CH1_RD_DMA_STAT_CLRr, &rval,
                                          FIFO_CH1_DMA_HOSTMEM_TIMEOUTf, 1);
                    soc_pci_write(unit, 
                              CMIC_CMCx_FIFO_CHy_RD_DMA_STAT_CLR_OFFSET(cmc, 
                                          SOC_MEM_FIFO_DMA_CHANNEL_1), rval);
                }
            }
#endif /* BCM_CMICM_SUPPORT */
        }

cleanup_exit:
    (void)soc_mem_fifo_dma_stop(unit, chan[0]);
#if defined (BCM_TRIUMPH_SUPPORT)
    if (mem[1] != INVALIDm) {
        (void)soc_mem_fifo_dma_stop(unit, chan[1]);
        (void)soc_reg_field32_modify(unit, ESM_CTLr, REG_PORT_ANY,
                                     L2MODFIFO_PUSH_ENf, 0);
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    if (host_buf[0] != NULL) {
        soc_cm_sfree(unit, host_buf[0]);
    }
    if (mem[1] != INVALIDm && host_buf[1] != NULL) {
        soc_cm_sfree(unit, host_buf[1]);
    }
    soc->l2x_pid = SAL_THREAD_ERROR;
    sal_thread_exit(0);
}
#endif /* BCM_TRX_SUPPORT */
#endif /* BCM_XGS3_SWITCH_SUPPORT */

/*
 * Function:
 * 	soc_l2mod_running
 * Purpose:
 *	Determine the L2MOD sync thread running parameters
 * Parameters:
 *	unit - unit number.
 *	flags (OUT) - if non-NULL, receives the current flag settings
 *	interval (OUT) - if non-NULL, receives the current pass interval
 * Returns:
 *   	Boolean; TRUE if L2MOD sync thread is running
 */

int
soc_l2mod_running(int unit, uint32 *flags, sal_usecs_t *interval)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    soc_control_t	*soc = SOC_CONTROL(unit);

    if (SOC_IS_XGS3_SWITCH(unit)) {
        if (soc->l2x_pid != SAL_THREAD_ERROR) {
            if (flags != NULL) {
                *flags = soc->l2x_flags;
            }
            if (interval != NULL) {
                *interval = soc->l2x_interval;
            }
        }

        return (soc->l2x_pid != SAL_THREAD_ERROR);
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return SOC_E_UNAVAIL;
}

/*
 * Function:
 * 	soc_l2mod_stop
 * Purpose:
 *   	Stop L2MOD-related thread
 * Parameters:
 *	unit - unit number.
 * Returns:
 *	SOC_E_XXX
 */

int
soc_l2mod_stop(int unit)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    soc_control_t * soc = SOC_CONTROL(unit);
    int			rv = SOC_E_NONE;

    if (SOC_IS_XGS3_SWITCH(unit)) {
        soc_cm_debug(DK_ARL, "soc_l2mod_stop: unit=%d\n", unit);

        _soc_l2mod_fifo_enable(unit, 0);
        if (!soc_feature(unit, soc_feature_fifo_dma)) {
            soc_intr_disable(unit, IRQ_L2_MOD_FIFO_NOT_EMPTY);
            /* Wake up thread so it will check the exit flag */
            sal_sem_give(soc->arl_notify);
        }
#if defined (BCM_TRIUMPH_SUPPORT)
        if (soc_feature(unit, soc_feature_esm_support)) {
            SOC_IF_ERROR_RETURN
               (soc_reg_field32_modify(unit, ESM_L2_AGE_CTLr, REG_PORT_ANY,
                                       CPU_NOTIFYf, 0));
            SOC_IF_ERROR_RETURN
               (soc_reg_field32_modify(unit, ESM_PER_PORT_REPL_CONTROLr,
                                       REG_PORT_ANY, CPU_NOTIFYf, 0));
        }
#endif /* BCM_TRIUMPH_SUPPORT */
        return rv;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return SOC_E_UNAVAIL;
}

int
soc_l2mod_start(int unit, uint32 flags, sal_usecs_t interval)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    soc_control_t * soc = SOC_CONTROL(unit);
    int             pri;

    if (SOC_IS_XGS3_SWITCH(unit)) {
        if (!soc_feature(unit, soc_feature_arl_hashed)) {
            return SOC_E_UNAVAIL;
        }

        if (soc->l2x_interval != 0) {
            SOC_IF_ERROR_RETURN(soc_l2mod_stop(unit));
        }

        sal_snprintf(soc->l2x_name, sizeof (soc->l2x_name), "bcmL2MOD.%d", unit);

        soc->l2x_flags = flags;
        soc->l2x_interval = interval;

        if (interval == 0) {
            return SOC_E_NONE;
        }

        if (soc->l2x_pid == SAL_THREAD_ERROR) {
            pri = soc_property_get(unit, spn_L2XMSG_THREAD_PRI, 50);

#ifdef BCM_TRX_SUPPORT
            if (soc_feature(unit, soc_feature_fifo_dma)) {
                soc->l2x_pid =
                    sal_thread_create(soc->l2x_name, SAL_THREAD_STKSZ, pri,
                                      _soc_l2mod_dma_thread, INT_TO_PTR(unit));
            } else
#endif /* BCM_TRX_SUPPORT */
            {
                soc->l2x_pid =
                    sal_thread_create(soc->l2x_name, SAL_THREAD_STKSZ, pri,
                                      _soc_l2mod_thread, INT_TO_PTR(unit));
            }
            if (soc->l2x_pid == SAL_THREAD_ERROR) {
                soc_cm_debug(DK_ERR,
                             "soc_l2mod_start: Could not start L2MOD thread\n");
                return SOC_E_MEMORY;
            }
        }

        if (!soc_feature(unit, soc_feature_fifo_dma)) {
            soc_intr_enable(unit, IRQ_L2_MOD_FIFO_NOT_EMPTY);
        }
        _soc_l2mod_fifo_enable(unit, 1);

        return SOC_E_NONE;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    return SOC_E_UNAVAIL;
}
