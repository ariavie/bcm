/*
 * $Id:  Exp $
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
 * File:       fb4regex.c
 * Purpose:    Internal regex function prototypes.
 */

#ifndef _BCM_INT_FIREBOLT4_REGEX_H_
#define _BCM_INT_FIREBOLT4_REGEX_H_
#if defined(INCLUDE_REGEX)
#include <bcm/bregex.h>
#include <bcm_int/esw/bregex.h>

int _bcm_tr3_regex_init(int unit, _bcm_esw_regex_functions_t **functions);
int _bcm_esw_tr3_regex_sync(int unit);
int _bcm_tr3_regex_config_set(int unit, bcm_regex_config_t *config);
int _bcm_tr3_regex_config_get(int unit, bcm_regex_config_t *config);

int _bcm_tr3_regex_exclude_add(int unit, uint8 protocol, 
                            uint16 l4_start, uint16 l4_end);

int _bcm_tr3_regex_exclude_get(int unit, int array_size, uint8 *protocol, 
                        uint16 *l4low, uint16 *l4high, int *array_count);

int _bcm_tr3_regex_exclude_delete(int unit, uint8 protocol, 
                            uint16 l4_start, uint16 l4_end);

int _bcm_tr3_regex_exclude_delete_all(int unit);
int _bcm_tr3_regex_engine_create(int unit, bcm_regex_engine_config_t *config, 
                            bcm_regex_engine_t *engid);

int _bcm_tr3_regex_engine_destroy(int unit, bcm_regex_engine_t engid);

int _bcm_tr3_regex_engine_traverse(int unit, bcm_regex_engine_traverse_cb cb, 
                                void *user_data);
int _bcm_tr3_regex_engine_get(int unit, bcm_regex_engine_t engid, 
                            bcm_regex_engine_config_t *config);

int _bcm_tr3_regex_match_check(int unit,  bcm_regex_match_t* match,
                                int count, int* metric);

int _bcm_tr3_regex_match_set(int unit, bcm_regex_engine_t engid,
                              bcm_regex_match_t* match, int count);

int _bcm_esw_regex_report_register(int unit, uint32 reports,
                       bcm_regex_report_cb callback, void *userdata);

int _bcm_esw_regex_report_unregister(int unit, uint32 reports,
                         bcm_regex_report_cb callback, void *userdata);

int _bcm_tr3_get_engine_size_info(int unit, int engine_id,
                                  bcm_regex_engine_info_t *engine_info);

int _bcm_tr3_get_match_id(int unit, int signature_id, int *match_id);

int _bcm_tr3_get_sig_id(int unit, int match_id, int *signature_id);

int _bcm_tr3_regex_info_get(int unit, bcm_regex_info_t *regex_info);

int _bcm_tr3_regex_stat_get(int unit, bcm_regex_stat_t type, uint64 *val);

int _bcm_tr3_regex_stat_set(int unit, bcm_regex_stat_t type, uint64 val);

int _bcm_tr3_regex_session_add(int unit, int flags,
                               bcm_regex_session_key_t *key,
                               bcm_regex_session_t *session);

int _bcm_tr3_regex_session_policy_update(int unit, int flags, int flow_index,
                                         bcm_regex_policy_t policy);

int _bcm_tr3_regex_session_delete_all(int unit);

int _bcm_tr3_regex_session_delete(int unit,
                                  bcm_regex_session_key_t *key);

int _bcm_tr3_regex_session_get(int unit, int flags,
                               bcm_regex_session_key_t *key,
                               bcm_regex_session_t *session);

int _bcm_tr3_regex_session_traverse(int unit,
                                    int flags,
                                    bcm_regex_session_traverse_cb cb,
                                    void *user_data);
int _bcm_tr3_regex_dump_axp(int unit);
#endif  /* INCLUDE_REGEX */

int _bcm_esw_regex_sync(int unit);

#endif /* _BCM_INT_FIREBOLT4_REGEX_H_ */

