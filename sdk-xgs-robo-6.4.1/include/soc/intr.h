/*
 * $Id: intr.h,v 1.32 Broadcom SDK $
 *
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

#ifndef _SOC_INTER_H_
#define _SOC_INTER_H_

#include <soc/chip.h>
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#include <soc/mcm/allenum.h>
#endif

/* 
 * Defines
 */

#define SOC_INTERRUPT_INTERRUPT_PER_REG_NUM_MAX 32

#define SOC_INTERRUPT_BIT_FIELD_DONT_CARE                    -1
#define SOC_ACTIVE_INTERRUPTS_GET_UNMASKED_ONLY 0x1
#define SOC_ACTIVE_INTERRUPTS_GET_CONT_PREV 0x2

#define SOC_INTERRUPT_DB_FLAGS_PRINT_ENABLE              0 
#define SOC_INTERRUPT_DB_FLAGS_CORR_ACT_OVERRIDE_ENABLE  1 
#define SOC_INTERRUPT_DB_FLAGS_PRIORITY_BITS_LSB         2
#define SOC_INTERRUPT_DB_FLAGS_PRIORITY_BITS_LEN         4
#define SOC_INTERRUPT_DB_FLAGS_PRIORITY_MAX_VAL          15
#define SOC_INTERRUPT_DB_FLAGS_PRIORITY_MASK             (((1 << (SOC_INTERRUPT_DB_FLAGS_PRIORITY_BITS_LEN + SOC_INTERRUPT_DB_FLAGS_PRIORITY_BITS_LSB)) - 1) &(~(( 1 << SOC_INTERRUPT_DB_FLAGS_PRIORITY_BITS_LSB) -1))) 
#define SOC_INTERRUPT_DB_FLAGS_UNMASK_AND_CLEAR_DISABLE 0x40
#define SOC_INTERRUPT_DB_FLAGS_FORCE_UNMASK             0x80

/* 
 * Types Def
 */
 
 typedef struct soc_interrupt_clear_read_fifo_s {
    soc_reg_t fifo_reg;
    uint32    read_count;  
} soc_interrupt_clear_read_fifo_t;

typedef struct soc_interrupt_clear_reg_write_s {
    soc_reg_t status_reg;
    soc_reg_above_64_val_t data;  
} soc_interrupt_clear_reg_write_t;

typedef struct soc_interrupt_clear_array_index_s {
    soc_reg_t fifo_reg;
    uint32    read_count;  
    uint32    reg_index;
} soc_interrupt_clear_array_index_t;

typedef struct soc_interrupt_cause_s {
    int id;
    int index;
} soc_interrupt_cause_t;

typedef struct  soc_interrupt_tree_s {
    soc_reg_t int_reg;
    soc_reg_t int_mask_reg;
    int index;
    int int_id[SOC_INTERRUPT_INTERRUPT_PER_REG_NUM_MAX];
}soc_interrupt_tree_t;

typedef int (* clear_func)(int unit, int block_instance, int interrupt_id);

typedef struct soc_interrupt_db_s {
#if !defined(SOC_NO_NAMES)
    char *name;
#endif
    int id;
    soc_reg_t reg;
    soc_reg_t reg_test;
    int reg_index;
    soc_field_t field;
    soc_reg_t mask_reg;
    int mask_reg_index;
    soc_field_t mask_field;
    int vector_id;
    CONST soc_interrupt_tree_t *vector_info;
    soc_block_type_t block_type;
    clear_func interrupt_clear;

    /* for clear function */
    void *interrupt_clear_param1;
    void *interrupt_clear_param2;
    
    /* per block instance*/
    uint32          *statistics_count;
    uint32          *storm_detection_occurrences;
    uint32          *storm_detection_start_time;
    uint32          *storm_nominal_count;

    int bit_in_field;
} soc_interrupt_db_t;

typedef struct soc_interrupt_s {
    soc_interrupt_db_t *interrupt_db_info;
    soc_interrupt_tree_t *interrupt_tree_info;    /* interrupts array */
} soc_interrupt_t;

#define SOC_INTR_IS_SUPPORTED(unit) \
    (NULL != SOC_CONTROL(unit)->interrupts_info) 

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_DFE_SUPPORT)|| defined(BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT) || defined(PORTMOD_SUPPORT)
/* 
 * Functions
 */

/* Main interrupt handling SOC function */
void soc_cmn_block_error(void *unit_vp, void *d1, void *d2, void *d3, void *d4);
void soc_cmn_error(void *unit_vp, void *d1, void *d2, void *d3, void *d4);

/* Per interrupt functions*/
int soc_interrupt_is_valid(int unit, const soc_block_info_t* bi, const soc_interrupt_db_t* inter, int* is_valid /*out*/);
int soc_interrupt_get(int unit, int block_instance, const soc_interrupt_db_t* inter, int* inter_val /*out*/);
int soc_interrupt_force_get (int unit, int block_instance, const soc_interrupt_db_t* inter, int* inter_val);
int soc_interrupt_enable(int unit, int block_instance, const soc_interrupt_db_t* inter);
int soc_interrupt_force(int unit, int block_instance, const soc_interrupt_db_t* inter, int action);
int soc_interrupt_disable(int unit, int block_instance, const soc_interrupt_db_t* inter);
int soc_interrupt_is_enabled(int unit, int block_instance, const soc_interrupt_db_t* inter, int* is_enabled /*out*/);
int soc_interrupt_is_supported(int unit, int block_instance, int inter_id);
int soc_interrupt_stat_cnt_increase(int unit, int bi, int interrupt_id);

int soc_interrupt_flags_set(int unit, int interrupt_id, uint32 flags);
int soc_interrupt_flags_get(int unit, int interrupt_id, uint32* flags);

/* Global interrupt functions */
int soc_nof_interrupts(int unit, int* nof_interrupts);
int soc_interrupt_is_all_mask(int unit, int *is_all_mask);

/* get the index of the block from the blocks port and interrupt id*/
int soc_interrupt_get_block_index_from_port(int unit, int interrupt_id, int port);
/* get the port of  a interrupt from interrupt ID and block index. */
int soc_interrupt_get_intr_port_from_index(int unit, int interrupt_id, int block_instance);

/* Storm functions */
int soc_interrupt_storm_timed_period_set(int unit, int interrupt_id, uint32 storm_timed_period);
int soc_interrupt_storm_timed_period_get(int unit, int intrrupt_id, uint32* storm_timed_period);

int soc_interrupt_storm_timed_count_set(int unit, int interrupt_id, uint32 storm_timed_count);
int soc_interrupt_storm_timed_count_get(int unit, int interrupt_id, uint32* storm_timed_count);

int soc_interrupt_update_storm_detection(int unit, int block_instance, soc_interrupt_db_t *inter);
int soc_interrupt_is_storm(int unit, int block_instance, soc_interrupt_db_t *inter, int *is_storm_count_period,int *is_storm_nominal);

/* Clear functions */
int soc_interrupt_clear_on_write(int unit, int block_instance, int interrupt_id);
int soc_interrupt_clear_on_reg_write(int unit, int block_instance, int interrupt_id);
int soc_interrupt_clear_on_clear(int unit, int block_instance, int interrupt_id);
int soc_interrupt_clear_on_read_fifo(int unit, int block_instance, int interrupt_id);
int soc_interrupt_clear_on_read_array_index(int unit, int block_instance, int interrupt_id);
int soc_interrupt_clear_all(int unit);
int soc_interrupt_is_all_clear(int unit, int *is_all_clear);

/*
 *flags available:
 *      SOC_ACTIVE_INTERRUPTS_GET_ALL - returns all active interrupts
 *      SOC_ACTIVE_INTERRUPTS_GET_UNMASKED_ONLY - returns just active and unmasked interrupts.
 *      SOC_ACTIVE_INTERRUPTS_GET_CONT_PREV - continue checking from th place the function stopped last time.
 * 
 * output:
 *      total_interrupts - the number of active interrupts (all or unmasked).
 *      interrupts[1..min(max_interrupts_size, *total_interrupts) - 1] - <cause, index> of each active interrupts.
 *      if *total_interrupts > max_interrupts_size interrupts do not include all active interrupts -
 *          for more interrupts run again with the flag SOC_ACTIVE_INTERRUPTS_GET_CONT_PREV.
 * 
 */
extern int
soc_active_interrupts_get(int unit, int flags, int max_interrupts_size,
                          soc_interrupt_cause_t *interrupts,
                          int *total_interrupts);

/*
 * returns: inter which togther with block_instance identifies the bits of the interrupt. 
 * 
 */
extern int
soc_interrupt_info_get(int unit, int interrupt_id, soc_interrupt_db_t *inter);

/* find the interupt ID of the register, register index and field*/
extern int 
soc_get_interrupt_id(int unit, soc_reg_t reg, int reg_index, soc_field_t field, int bit_in_field, int* interrupt_id);
/* find the interrupt ID of register address, block and bit index */
extern int
soc_get_interrupt_id_specific(int unit, int reg_adress, int reg_block, int field_bit, int* interrupt_id);
/* Order interrupts according to priority */
extern int 
soc_sort_interrupts_according_to_priority(int unit, soc_interrupt_cause_t *interrupts, uint32 interrupts_size);

#endif  /* #if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_DFE_SUPPORT)|| defined(BCM_PETRA_SUPPORT) || defined(PORTMOD_SUPPORT)*/

#endif /* _SOC_INTER_H_ */  


