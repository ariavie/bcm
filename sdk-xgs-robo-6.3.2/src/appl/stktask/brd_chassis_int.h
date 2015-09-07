/* 
 * $Id: brd_chassis_int.h 1.7 Broadcom SDK $
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
 * File:        brd_chassis_int.h
 * Purpose:     Internal chassis support
 */

#ifndef   _BRD_CHASSIS_INT_H_
#define   _BRD_CHASSIS_INT_H_

#include <appl/cpudb/cpudb.h>
#include <appl/stktask/topology.h>

#if defined(INCLUDE_CHASSIS)

/* Chassis board programming */

extern int chassis_ast_cfm_xgs2(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_ast_xgs2_48g(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_ast_xgs2_6x(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_ast_xgs3_12x(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_ast_xgs3_48g(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_ast_cfm_xgs3(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_ast_56800_12x(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);

extern int chassis_smlb_cfm_xgs2(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_smlb_xgs2_48g(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_smlb_xgs2_6x(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_smlb_xgs3_12x(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_smlb_xgs3_48g(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_smlb_cfm_xgs3(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_smlb_56800_12x(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);

extern int chassis_cfm_xgs2(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_cfm_xgs3(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_lm6x(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_lm_xgs2_48g(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_lm_xgs3_48g(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_lm_xgs3_12x(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);
extern int chassis_lm_56800_12x(topo_cpu_t *tp_cpu, cpudb_ref_t db_ref);

/* Chassis common functions */

#define LB_FLAGS (CPUDB_BASE_F_CHASSIS | \
                  CPUDB_BASE_F_LOAD_BALANCE | \
                  CPUDB_BASE_F_CHASSIS_AST)

#define CHASSIS_AST(flag) ((flag & LB_FLAGS) == LB_FLAGS)

#define CHASSIS_SM(flag)  ((flag & LB_FLAGS) == \
                          (CPUDB_BASE_F_CHASSIS | CPUDB_BASE_F_LOAD_BALANCE))


/* Assumes CFM slot IDs are consecutive and less than LM slot IDs */
#define NUM_CFM                 2
#define NUM_LM                  8
#define CFM_SLOT_OFFSET         0
#define LM_SLOT_OFFSET         (CFM_SLOT_OFFSET + NUM_CFM)
#define NUM_SLOT               (NUM_CFM + NUM_LM)

#define MODIDS_PER_CFM          1
#define MODIDS_PER_LM           6

#define IS_CFM_SLOT(slot)      ((slot >= CFM_SLOT_OFFSET) \
                               && (slot < LM_SLOT_OFFSET))

#define IS_LM_SLOT(slot)       ((slot >= LM_SLOT_OFFSET) \
                               && (slot < (LM_SLOT_OFFSET + NUM_LM)))

#define LM_SLOT_ID             LM_SLOT_OFFSET

typedef struct bcm_board_cfm_data_s {
    int          slot;       /* Slot id */
    bcm_module_t modid;      /* Mod id */
} bcm_board_cfm_data_t;

typedef struct bcm_board_cfm_info_s {
    int count;          /* Number of valid entries */
    int master;         /* Index of master info in cfm[] */
    int is_master;      /* True if caller is a CFM master */
    bcm_board_cfm_data_t cfm[NUM_CFM];
} bcm_board_cfm_info_t;


/* Map from slot-id to base module ID */
extern int topo_chassis_slot_to_mod_base[NUM_SLOT];

/* Get first mod-id on the given slot (CFM or LM) */
#define MODID_OFFSET_GET(_slot)    (topo_chassis_slot_to_mod_base[_slot])

/* Get number of mod-ids reserved for given slot (CFM or LM) */
#define NUM_MODIDS_GET(_slot)  \
    ((IS_CFM_SLOT(_slot)) ? MODIDS_PER_CFM : MODIDS_PER_LM)

/* Get CFM mod-id (only one) */
#define CFM_MODID(_slot)     (MODID_OFFSET_GET(_slot))

/* Iterate thru all LMs */
#define FOREACH_LM_SLOT(_slot)    \
    for (_slot = LM_SLOT_OFFSET; _slot < NUM_SLOT; _slot++)

/* Iterate thru mod-ids on the given slot (LM or CFM) */
#define FOREACH_MODID(_slot, _modid)                                 \
    for (_modid = MODID_OFFSET_GET(_slot);                           \
         _modid < (MODID_OFFSET_GET(_slot) + NUM_MODIDS_GET(_slot)); \
         _modid++)

/* Iterate thru mod-ids on all other LMs */
#define FOREACH_OTHER_MODID(_this, _slot, _mod) \
    FOREACH_LM_SLOT(_slot)                      \
        if (_this != _slot)                     \
            FOREACH_MODID(_slot, _mod)


typedef int (*modport_set_f) (int unit, int dst_modid, bcm_port_t stk_port);
typedef int (*int_sp_set_f)  (int unit, int dst_unit, bcm_port_t stk_port);

/* List of stack ports */
#define STK_PORTS_MAX    CPUDB_STK_PORTS_MAX
typedef struct {
    int             port_cnt;
    bcm_port_t      port[STK_PORTS_MAX];
} sp_list_t;

/* Defines destination (unit or slot) and the corresponding stack port list */
typedef struct {
    int              dst;
    sp_list_t        sp;
} dst_t;

/* Defines a unit to local units connectivity */
typedef struct {
    modport_set_f    modport_set_fn;
    int_sp_set_f     internal_sp_set_fn;
    int              unit_cnt;
    dst_t            unit[BCM_LOCAL_UNITS_MAX];
} to_unit_t;

/* Defines a unit to slot connectivity */
/* Notes: allows external or internal stack ports */
typedef struct {
    modport_set_f    modport_set_fn;
    int              slot_cnt;
    dst_t            slot[NUM_LM];  /* Assumes more LM than CFM slots */
} to_slot_t;


#define ENABLE_FLOOD          0
#define DISABLE_FLOOD         (BCM_PORT_FLOOD_BLOCK_BCAST |         \
                               BCM_PORT_FLOOD_BLOCK_UNKNOWN_UCAST | \
                               BCM_PORT_FLOOD_BLOCK_UNKNOWN_MCAST)


extern int bcm_board_cfm_info(cpudb_ref_t db_ref, bcm_board_cfm_info_t *info);
extern int _bcm_board_setup_trunk_lm_xgs3_48g(void);
extern int bcm_board_chassis_setup(cpudb_ref_t db_ref);
extern int topo_chassis_modids_assign(cpudb_ref_t db_ref);

#endif /* INCLUDE_CHASSIS  */




#endif /* _BRD_CHASSIS_INT_H_ */
