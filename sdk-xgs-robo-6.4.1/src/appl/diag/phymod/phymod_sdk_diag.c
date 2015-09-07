/*
 * $Id: 151551713cd5ee02238de1a7e70a168aab1393ae $ 
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

#include <appl/diag/parse.h>
#include <appl/diag/system.h>
#include <shared/bsl.h>
#ifdef PHYMOD_DIAG

#include <phymod/phymod_diag.h>
#include <phymod/phymod.h>
#include <phymod/phymod_diagnostics.h>
#include <appl/diag/phymod/phymod_sdk_diag.h>

#define MAX_PHYS_ACCESS_STRUCTURES (144)
#define MAX_PHYS_STRUCTS_PER_PORT (6)

STATIC int
enum_parse(enum_mapping_t *mapping, char *str, uint32_t *val)
{
    char *key;
    int i = 0;

    key = mapping[0].key;
    while (key != NULL) {
        if (sal_strcasecmp(key, str) == 0) {
            *val = mapping[i].enum_value;
            return CMD_OK;
        }
        i++;
        key = mapping[i].key;
    }
    return CMD_FAIL;
}


/******************************************************************************
 * DSC
 ******************************************************************************/
#define DSC_USAGE   "dsc <port>\n"

STATIC cmd_result_t
phymod_sdk_diag_dsc_wrapper(phymod_phy_access_t *phys, int array_size,  args_t *args)
{
    int res;
    
    res = phymod_diag_dsc(phys, array_size);
    return (res == PHYMOD_E_NONE)? CMD_OK : CMD_FAIL;
}

/******************************************************************************
 * REG read/write
 ******************************************************************************/
#define REG_READ_USAGE  "read <port> <addr>\n"
#define REG_WRITE_USAGE "write <port> <addr> <val>\n"

STATIC cmd_result_t
phymod_sdk_diag_reg_read_wrapper(phymod_phy_access_t *phys, int array_size,  args_t *args)
{
    char *str;
    uint32_t reg_addr;
    int res = PHYMOD_E_NONE;

    if ((str = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    if(isint(str)){
        reg_addr = parse_integer(str);
        res = phymod_diag_reg_read(phys, array_size, reg_addr);
    } else{
        return CMD_USAGE;
    }
    
    return (res == PHYMOD_E_NONE)? CMD_OK : CMD_FAIL;
}

STATIC cmd_result_t
phymod_sdk_diag_reg_write_wrapper(phymod_phy_access_t *phys, int array_size,  args_t *args)
{
    char *str;
    uint32_t reg_addr;
    uint32_t val;
    int res;

    if ((str = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    if(isint(str)){
        reg_addr = parse_integer(str);
            if ((str = ARG_GET(args)) == NULL) {
                return CMD_USAGE;
            }
        val = parse_integer(str);        
        res = phymod_diag_reg_write(phys, array_size, reg_addr, val);
    }else{
        return CMD_USAGE;
    }

    return (res == PHYMOD_E_NONE)? CMD_OK : CMD_FAIL;
}


/******************************************************************************
 * PRBS
 ******************************************************************************/
#define PRBS_USAGE \
    "prbs diagnostics usage:\n" \
    "prbs <ports> clear [rx] [tx]\n" \
    "prbs <ports> get <time_to_wait>\n" \
    "prbs <ports> set <Polynomial=<phymodPrbsPolyX>> [loopback] [enable] [invert] [rx] [tx]\n" \
    "\ndefault values: loopback=false, enable=true, invert=false rx=flase tx=false\n" \
    "note: when nor rx neither tx flags are set the PRBS set/clear operation will be performed on both\n"

STATIC int
phymod_sdk_diag_prbs_get_args_parse(args_t *args, phymod_diag_prbs_get_args_t *prbs_get_args)
{
    char *str;

    if ((str = ARG_GET(args)) == NULL) {
        return CMD_USAGE;
    }
    prbs_get_args->time = parse_integer(str);
    return CMD_OK;
}

STATIC int
phymod_sdk_diag_prbs_set_args_parse(args_t *args, phymod_diag_prbs_set_args_t *prbs_set_args)
{
    parse_table_t pt;
    char *poly;
    int rx_flag;
    int tx_flag;

    parse_table_init(0, &pt); /* Unit meaning leess in parse table */
    parse_table_add(&pt, "polynomial", PQ_STRING, (void *)0, &poly, NULL);
    parse_table_add(&pt, "loopback",  PQ_VAL | PQ_BOOL | PQ_NO_EQ_OPT, (void *)0, &prbs_set_args->loopback, NULL);
    parse_table_add(&pt, "enable", PQ_VAL | PQ_BOOL | PQ_NO_EQ_OPT, (void *)1, &prbs_set_args->enable, NULL);
    parse_table_add(&pt, "invert", PQ_VAL | PQ_BOOL | PQ_NO_EQ_OPT, (void *)0, &prbs_set_args->prbs_options.invert, NULL);
    parse_table_add(&pt, "rx", PQ_VAL | PQ_BOOL | PQ_NO_EQ_OPT, (void *)0, &rx_flag, NULL);
    parse_table_add(&pt, "tx", PQ_VAL | PQ_BOOL | PQ_NO_EQ_OPT, (void *)0, &tx_flag, NULL);
    if (parse_arg_eq(args, &pt) < 0) {
        cli_out("ERROR: invalid option: %s\n", ARG_CUR(args));
        parse_arg_eq_done(&pt);
        return CMD_USAGE;
    }
    if (poly == NULL || sal_strlen(poly) == 0) {
        parse_arg_eq_done(&pt);
        cli_out("missing parameter: polynomial\n");       
        return CMD_USAGE;
    }
    if (enum_parse(phymod_prbs_poly_t_mapping, poly, &prbs_set_args->prbs_options.poly) != CMD_OK) {
        cli_out("invalid polynomial %s\n", poly);
        parse_arg_eq_done(&pt);
        return CMD_USAGE;
    }
    /* Now free allocated strings */
    parse_arg_eq_done(&pt);

    if (rx_flag) {
        PHYMOD_PRBS_DIRECTION_RX_SET(prbs_set_args->flags);
    }
    if (tx_flag) {
        PHYMOD_PRBS_DIRECTION_TX_SET(prbs_set_args->flags);
    }
    return CMD_OK;   
}

STATIC int
phymod_sdk_diag_prbs_clear_args_parse(args_t *args, phymod_diag_prbs_clear_args_t *prbs_clear_args)
{
    parse_table_t pt;
    int rx_flag;
    int tx_flag;

    parse_table_init(0, &pt); /* Unit meaning leess in parse table */
    parse_table_add(&pt, "rx", PQ_VAL | PQ_BOOL | PQ_NO_EQ_OPT, (void *)0, &rx_flag, NULL);
    parse_table_add(&pt, "tx", PQ_VAL | PQ_BOOL | PQ_NO_EQ_OPT, (void *)0, &tx_flag, NULL);
    if (parse_arg_eq(args, &pt) < 0) {
        cli_out("ERROR: invalid option: %s\n", ARG_CUR(args));
        return CMD_USAGE;
    }
    if (rx_flag) {
        PHYMOD_PRBS_DIRECTION_RX_SET(prbs_clear_args->flags);
    }
    if (tx_flag) {
        PHYMOD_PRBS_DIRECTION_TX_SET(prbs_clear_args->flags);
    }
    return CMD_OK;   
}

STATIC int
phymod_sdk_diag_prbs_args_parse(args_t *args,  phymod_diag_prbs_args_t *prbs_args)
{
    enum_mapping_t phymod_prbs_cmd[] = {
        {"clear", PhymodDiagPrbsClear},
        {"set", PhymodDiagPrbsSet},
        {"get",PhymodDiagPrbsGet},
        {NULL, 0}
    };
    int rv = CMD_OK;
    char *operation;

    operation = ARG_GET(args);
    if (operation == NULL) {
        return CMD_USAGE;
    }
    if (enum_parse(phymod_prbs_cmd, operation, &prbs_args->prbs_cmd) != CMD_OK) {
        cli_out("Invalid operation type %s\n", operation);
        return CMD_USAGE;
    }
    switch(prbs_args->prbs_cmd) {
    case PhymodDiagPrbsClear:
        /* No additional parameters */
        rv= phymod_sdk_diag_prbs_clear_args_parse(args, &prbs_args->args.clear_params);
        break;
    case PhymodDiagPrbsGet:
        rv = phymod_sdk_diag_prbs_get_args_parse(args, &prbs_args->args.get_params);
        break;
    case PhymodDiagPrbsSet:
        rv = phymod_sdk_diag_prbs_set_args_parse(args, &prbs_args->args.set_params);
        break;
    default:
        /* Not supposed to get here */
        return CMD_FAIL;
    }
    return rv;
   
}

STATIC cmd_result_t
phymod_sdk_diag_prbs_wrapper(phymod_phy_access_t *phys, int array_size,  args_t *args)
{
    phymod_diag_prbs_args_t prbs_diag_args;
    int rv;
    int res;

    rv = phymod_sdk_diag_prbs_args_parse(args, &prbs_diag_args);
    if (rv !=  CMD_OK) {
        return rv;
    }
    res = phymod_diag_prbs(phys, array_size, &prbs_diag_args);
    return (res == PHYMOD_E_NONE)? CMD_OK : CMD_FAIL;
}

/******************************************************************************
 * General
 ******************************************************************************/
typedef cmd_result_t (*cmd_func_f)(phymod_phy_access_t *phys, int array_size,  args_t *args);


typedef struct phymod_cmd_s {
    char* c_cmd;
    cmd_func_f  c_func;
    char* c_usage;
} phymod_cmd_t;

static phymod_cmd_t commands[] ={
    {"prbs",  phymod_sdk_diag_prbs_wrapper, PRBS_USAGE},
    {"read", phymod_sdk_diag_reg_read_wrapper, REG_READ_USAGE},
    {"write", phymod_sdk_diag_reg_write_wrapper, REG_WRITE_USAGE},
    {"dsc", phymod_sdk_diag_dsc_wrapper, DSC_USAGE},
};

STATIC void
phymod_sdk_commands_print(void)
{
    int i = 0;

    cli_out("command types:\n");
    for (i = 0; i < COUNTOF(commands); i++) {
        cli_out("  %s\n", commands[i].c_cmd);
    }
}


STATIC int 
phymod_sdk_diag_parse_ports_str(int unit, char *port_str, bcm_pbmp_t *pbmp)
{
    BCM_PBMP_CLEAR(*pbmp);
    if (parse_bcm_pbmp(unit, port_str, pbmp) < 0) {
        cli_out("ERROR: unrecognized port bitmap: %s\n", port_str);
        return CMD_FAIL;
    }
    return CMD_OK;
}


cmd_result_t
phymod_sdk_diag(int unit, phymod_diag_map_func_f port_map ,args_t *args)
{
    int i = 0;
    int rv;
    int port;
    int command_found = FALSE;
    bcm_pbmp_t ports_bmp;
    int access_array_size = 0;
    phymod_phy_access_t access_array[MAX_PHYS_ACCESS_STRUCTURES];
    int nof_port_phy_accesses = 0;
    char *command;
    char *ports_str;
    
    /* Register the print function */
    phymod_diag_print_func = bsl_printf;

    command = ARG_GET(args);
    if (command == NULL) {
        phymod_sdk_commands_print();
        return CMD_NFND;
    }

    ports_str = ARG_GET(args);
    if (ports_str == NULL) {
        cli_out("missing port parameter\n");
        return CMD_NFND;
    }

    if(phymod_sdk_diag_parse_ports_str(unit, ports_str, &ports_bmp) != CMD_OK){
        cli_out("error parsing port parameter\n");
        return CMD_NFND;
    }
     BCM_PBMP_ITER(ports_bmp, port){
        nof_port_phy_accesses = 0;
        rv = port_map(unit, port, MAX_PHYS_STRUCTS_PER_PORT, &access_array[access_array_size], &nof_port_phy_accesses);
        if (rv != CMD_OK) {
            cli_out("error in translate port %d to phy access\n", port);
            return CMD_FAIL;
        }
        access_array_size += nof_port_phy_accesses;
     }

    for (i = 0; i < COUNTOF(commands); i++) {
        if (sal_strcasecmp(commands[i].c_cmd, command) == 0) {
            /* Command found */
            command_found = TRUE;
            break;
        }
    }
    if(command_found){
        rv = commands[i].c_func(access_array, access_array_size, args);
        if (rv == CMD_USAGE) {
            cli_out("%s", commands[i].c_usage);
            return CMD_FAIL;
        }
    }
    else{
        phymod_sdk_commands_print();
        return CMD_NFND;
    }
    return CMD_OK;
}

#endif /* PHYMOD_DIAG */

int _phymod_sdk_diag_not_empty;
