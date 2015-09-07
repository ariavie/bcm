/*
 * $Id: txbeacon.c,v 1.8 Broadcom SDK $
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

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <shared/bsl.h>
#include <bcm/error.h>
#include <bcm/txbeacon.h>

#define CLEAN_UP_AND_RETURN(_result) \
    parse_arg_eq_done(&parse_table); \
    return (_result);

char cmd_esw_txbeacon_usage[] = 
#ifdef COMPILER_STRING_CONST_LIMIT
    "Usage: txbeacon <option> [args...]\n"
#else
    "\n"
    "  txbeacon init\n"
    "  txbeacon add [PORT=<id>] [FLAGS=<flags>] [LEN=<len>] [MAXLEN=<maxlen>]\n"
    "                   [PKTID=<pkt_id>] [INTERVAL=<int>] [COUNT=<count>]\n"
    "                   [DATA=<repeat val>]\n"
#endif
    ;

int
_txb_cb(int unit, bcm_txbeacon_t *tx, void *user_data)
{
    cli_out("Got user=%p pktid=%x flags=%x len=%x port=%x interval=%x count=%x pkt_data=%x\n",
            user_data, tx->pkt_id, tx->flags, tx->len, tx->port, tx->interval, tx->count, tx->pkt_data[0]);
    return (SOC_E_NONE);
}

cmd_result_t cmd_esw_txbeacon(int unit, args_t *args)
{
    char *arg_string_p = NULL;
    parse_table_t parse_table;
    int result;
    
    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL) {
        return CMD_USAGE;
    }

    if (!sh_check_attached(ARG_CMD(args), unit)) {
        return CMD_FAIL;
    }

    if (sal_strcasecmp(arg_string_p, "init") == 0) {
        result = bcm_txbeacon_init(unit, 0);

        if (BCM_FAILURE(result)) {
            cli_out("Command failed.  %s.\n", bcm_errmsg(result));

            return CMD_FAIL;
        }

        cli_out("TXBeacon module initialized.\n");
        
    } else if (sal_strcasecmp(arg_string_p, "add") == 0) {
        uint32 flags, len, maxlen, pkt_id, port, interval, count, data, i;
        bcm_txbeacon_t txb;
        parse_table_init(unit, &parse_table);

        parse_table_add(&parse_table, "FLAGS", PQ_INT,
                        0, &flags, NULL);

        parse_table_add(&parse_table, "LEN", PQ_INT,
                        0, &len, NULL);

        parse_table_add(&parse_table, "MAXLEN", PQ_INT,
                        0, &maxlen, NULL);

        parse_table_add(&parse_table, "PKT_ID", PQ_INT,
                        0, &pkt_id, NULL);

        parse_table_add(&parse_table, "PORT", PQ_INT,
                        0, &port, NULL);

        parse_table_add(&parse_table, "INTERVAL", PQ_INT,
                        0, &interval, NULL);

        parse_table_add(&parse_table, "COUNT", PQ_INT,
                        0, &count, NULL);

        parse_table_add(&parse_table, "DATA", PQ_INT,
                        0, &data, NULL);

        if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
        {
            cli_out("Invalid option: %s\n", ARG_CUR(args));

            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        txb.flags = flags;
        txb.len = len;
        txb.maxlen = maxlen;
        txb.pkt_id = pkt_id;
        txb.port = port;
        txb.interval = interval;
        txb.count = count;
        
        txb.pkt_data = sal_alloc(len, "TXB test packet");
        for (i = 0; i < len; i++) {
            txb.pkt_data[i] = (data + i) & 0xff;
        } 
        
        result = bcm_txbeacon_pkt_setup(unit, &txb);
        
        if (BCM_FAILURE(result))
        {
            cli_out("Command failed.  %s.\n", bcm_errmsg(result));
        
            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        parse_arg_eq_done(&parse_table);

        sal_free(txb.pkt_data);

        return CMD_OK;            

    } else if (sal_strcasecmp(arg_string_p, "get") == 0) {
        uint32 maxlen, pkt_id;
        bcm_txbeacon_t txb;
        parse_table_init(unit, &parse_table);

        parse_table_add(&parse_table, "MAXLEN", PQ_INT,
                        0, &maxlen, NULL);

        parse_table_add(&parse_table, "PKT_ID", PQ_INT,
                        0, &pkt_id, NULL);

        if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
        {
            cli_out("Invalid option: %s\n", ARG_CUR(args));

            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        txb.maxlen = maxlen;
        txb.pkt_id = pkt_id;
        txb.pkt_data = sal_alloc(maxlen, "TXB test packet");
        
        result = bcm_txbeacon_pkt_get(unit, &txb);
        
        if (BCM_FAILURE(result))
        {
            cli_out("Command failed.  %s.\n", bcm_errmsg(result));
        
            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        cli_out("Got flags=%x len=%x port=%x interval=%x count=%x pkt_data=%x\n",
                txb.flags, txb.len, txb.port, txb.interval, txb.count, txb.pkt_data[0]);

        parse_arg_eq_done(&parse_table);

        sal_free(txb.pkt_data);
        
        return CMD_OK;            

    } else if (sal_strcasecmp(arg_string_p, "start") == 0) {
        uint32 pkt_id;
        parse_table_init(unit, &parse_table);

        parse_table_add(&parse_table, "PKT_ID", PQ_INT,
                        0, &pkt_id, NULL);

        if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
        {
            cli_out("Invalid option: %s\n", ARG_CUR(args));

            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        
        result = bcm_txbeacon_start(unit, pkt_id);
        
        if (BCM_FAILURE(result))
        {
            cli_out("Command failed.  %s.\n", bcm_errmsg(result));
        
            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        return CMD_OK;            

    } else if (sal_strcasecmp(arg_string_p, "stop") == 0) {
        uint32 pkt_id;
        parse_table_init(unit, &parse_table);

        parse_table_add(&parse_table, "PKT_ID", PQ_INT,
                        0, &pkt_id, NULL);

        if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
        {
            cli_out("Invalid option: %s\n", ARG_CUR(args));

            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        
        result = bcm_txbeacon_stop(unit, pkt_id);
        
        if (BCM_FAILURE(result))
        {
            cli_out("Command failed.  %s.\n", bcm_errmsg(result));
        
            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        return CMD_OK;            

        
    } else if (sal_strcasecmp(arg_string_p, "destroy") == 0) {
        uint32 pkt_id;
        parse_table_init(unit, &parse_table);

        parse_table_add(&parse_table, "PKT_ID", PQ_INT,
                        0, &pkt_id, NULL);

        if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
        {
            cli_out("Invalid option: %s\n", ARG_CUR(args));

            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        
        result = bcm_txbeacon_destroy(unit, pkt_id);
        
        if (BCM_FAILURE(result))
        {
            cli_out("Command failed.  %s.\n", bcm_errmsg(result));
        
            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        return CMD_OK;            

        
    } else if (sal_strcasecmp(arg_string_p, "scan") == 0) {
        parse_table_init(unit, &parse_table);   /* Init for CLEAN_UP.. */
        
        result = bcm_txbeacon_traverse(unit, &_txb_cb, (void *) 123);
        
        if (BCM_FAILURE(result))
        {
            cli_out("Command failed.  %s.\n", bcm_errmsg(result));
        
            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        return CMD_OK;            
        
    } else {
        cli_out("Invalid TXBEACON subcommand: %s\n", arg_string_p);

        return CMD_FAIL;
    }

    return CMD_OK;
}

