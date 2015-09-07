/*
 * $Id: cosq.c,v 1.3 Broadcom SDK $
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

#include <shared/bsl.h>

#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#include <bcm/error.h>
#include <bcm/cosq.h>
#include <bcm/debug.h>


static int 
_cosq_gport_traverse_cb(
    int unit, 
    bcm_gport_t port, 
    int numq, 
    uint32 flags, 
    bcm_gport_t gport, 
    void *user_data)
{
    int rv = 0;
    int i;
    int port_link;
    uint32 min;
    uint32 max;
    
    if (numq > 0) {
        for (i = 0; i <numq; i++) {        
            rv = bcm_cosq_gport_size_get(unit, gport, i, &min, &max);
            if (BCM_FAILURE(rv)) {
                return rv; 
            }
            port_link =  BCM_GPORT_LOCAL_GET(port);
            cli_out("port_idx:%2d queue_idx:%2d size:%8d schedule:strict\n",
                    port_link, i, max);
        }
    }
    return 0;
}

cmd_result_t
cmd_ea_cos(int unit, args_t *a)
{
    char *subcmd = NULL;
    char *threshold_cmd = NULL;
    int rv = 0;
    cmd_result_t ret_code = 0;
    parse_table_t pt;
    int arg_port = -1;
    int arg_count = -1;
    int arg_queue = -1;
    bcm_gport_t port;
    bcm_gport_t gport;
    bcm_cosq_report_threshold_t threshold;
    bcm_cosq_report_threshold_t org_threshold;
    
    if ((subcmd = ARG_GET(a)) == NULL) {
        return CMD_USAGE;
    }
    /* Check valid device to operation on ...*/
    if (! sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }
        
    if (!sal_strcasecmp(subcmd, "show")) {
        rv = bcm_cosq_gport_traverse(unit, _cosq_gport_traverse_cb, NULL);
        if (BCM_FAILURE(rv)) {
            cli_out("%s ERROR: %s\n", ARG_CMD(a), bcm_errmsg(rv));
            return CMD_FAIL;
        }
        return CMD_OK;
        
    } else if (!sal_strcasecmp(subcmd, "qcount")) { 
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Port",  PQ_DFL|PQ_PORT,
                0, &arg_port,   NULL);
        parse_table_add(&pt, "Count", PQ_DFL|PQ_INT,
                0, &arg_count,   NULL);
                
        if (ARG_CNT(a) < 2) {
            return CMD_USAGE;
        }
        if (!parseEndOk(a, &pt, &ret_code)) {
            return ret_code;
        }
        if ((arg_port == -1) || (arg_count == -1)) {
            return CMD_USAGE;
        }
        BCM_GPORT_LOCAL_SET(port, arg_port);
        rv = bcm_cosq_gport_add(unit, port, arg_count, 
                BCM_COSQ_GPORT_UCAST_QUEUE_GROUP, &gport);
        if (BCM_FAILURE(rv)) {
            cli_out("%s ERROR: %s\n", ARG_CMD(a), bcm_errmsg(rv));
            return CMD_FAIL;
        }
        return CMD_OK;
        
    } else if (!sal_strcasecmp(subcmd, "qsize")) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "Port",  PQ_DFL|PQ_PORT,
                0, &arg_port,   NULL);
        parse_table_add(&pt, "Queue", PQ_DFL|PQ_INT,
                0, &arg_queue,   NULL);
        parse_table_add(&pt, "Size", PQ_DFL|PQ_INT,
                0, &arg_count,   NULL);        
        if (ARG_CNT(a) < 3) {
            return CMD_USAGE;
        }
        if (!parseEndOk(a, &pt, &ret_code)) {
            return ret_code;
        }
        if ((arg_port == -1) || (arg_queue == -1) || (arg_count == -1)) {
            return CMD_USAGE;
        }  
        
        if  (arg_port < 1) {
            cli_out("%s ERROR: %s\n", ARG_CMD(a), bcm_errmsg(BCM_E_PARAM));
            return CMD_FAIL;
        }
        
        BCM_GPORT_UCAST_QUEUE_GROUP_SET(gport, arg_port - 1);
        rv = bcm_cosq_gport_size_set(unit, gport, arg_queue, 
                arg_count, arg_count);
        if (BCM_FAILURE(rv)) {
            cli_out("%s ERROR: %s\n", ARG_CMD(a), bcm_errmsg(rv));
            return CMD_FAIL;
        }
        return CMD_OK; 
    } else if (!sal_strcasecmp(subcmd, "qthreshold")) {
        
        if ((threshold_cmd = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
        if (!sal_strcasecmp(threshold_cmd, "show")) {
            parse_table_init(unit, &pt);
            parse_table_add(&pt, "Port",  PQ_DFL|PQ_PORT,
                    0, &arg_port,   NULL);
            parse_table_add(&pt, "Queue",  PQ_DFL|PQ_INT,
                    0, &arg_queue,   NULL);
                    
            if (ARG_CNT(a) < 2) {
                return CMD_USAGE;
            }
            if (!parseEndOk(a, &pt, &ret_code)) {
                return ret_code;
            }
            if  ((arg_port < 1) || ((arg_queue == -1))) {
                cli_out("%s ERROR: %s\n", ARG_CMD(a), bcm_errmsg(BCM_E_PARAM));
                return CMD_FAIL;
            }
            
            BCM_GPORT_UCAST_QUEUE_GROUP_SET(gport, arg_port - 1);
            rv = bcm_cosq_gport_report_threshold_get(unit, gport, arg_queue, 
                    &threshold);
            if (BCM_FAILURE(rv)) {
                cli_out("%s ERROR: %s\n", ARG_CMD(a), bcm_errmsg(rv));
                return CMD_FAIL;
            }
            cli_out("Threshold0: %d\n", threshold.threshold0);
            cli_out("Threshold1: %d\n", threshold.threshold1);
            cli_out("Threshold2: %d\n", threshold.threshold2);
            cli_out("Threshold3: %d\n", threshold.threshold3);
            return CMD_OK; 
            
        } else if (!sal_strcasecmp(threshold_cmd, "set")) {
            threshold.threshold0 = -1;
            threshold.threshold1 = -1;
            threshold.threshold2 = -1;
            threshold.threshold3 = -1;
            
            parse_table_init(unit, &pt);
            parse_table_add(&pt, "Port",  PQ_DFL|PQ_PORT,
                    0, &arg_port,   NULL);
            parse_table_add(&pt, "Queue", PQ_DFL|PQ_INT,
                    0, &arg_queue,   NULL);
            parse_table_add(&pt, "Threshold0", PQ_DFL|PQ_INT,
                    0, &threshold.threshold0,   NULL);
            parse_table_add(&pt, "Threshold1", PQ_DFL|PQ_INT,
                    0, &threshold.threshold1,   NULL);     
            parse_table_add(&pt, "Threshold2", PQ_DFL|PQ_INT,
                    0, &threshold.threshold2,   NULL); 
            parse_table_add(&pt, "Threshold3", PQ_DFL|PQ_INT,
                    0, &threshold.threshold3,   NULL); 
            if (ARG_CNT(a) < 3) {
                return CMD_USAGE;
            } 
            if (!parseEndOk(a, &pt, &ret_code)) {
                return ret_code;
            }
            if ((arg_port < 1) || (arg_queue == -1) || 
                (threshold.threshold0 == -1)) {
                return CMD_USAGE;   
            }
            
            BCM_GPORT_UCAST_QUEUE_GROUP_SET(gport, arg_port - 1);
            rv = bcm_cosq_gport_report_threshold_get(unit, gport, arg_queue, 
                    &org_threshold);
            if (BCM_FAILURE(rv)) {
                cli_out("%s ERROR: %s\n", ARG_CMD(a), bcm_errmsg(rv));
                return CMD_FAIL;
            }
            if (threshold.threshold0 == -1) {
                threshold.threshold0 = org_threshold.threshold0;
            } 
            if (threshold.threshold1 == -1) {
                threshold.threshold1 = org_threshold.threshold1;
            }
            if (threshold.threshold2 == -1) {
                threshold.threshold2 = org_threshold.threshold2;
            }
            if (threshold.threshold3 == -1) {
                threshold.threshold3 = org_threshold.threshold3;
            }
            rv = bcm_cosq_gport_report_threshold_set(unit, gport, arg_queue, 
                    &threshold);
            if (BCM_FAILURE(rv)) {
                cli_out("%s ERROR: %s\n", ARG_CMD(a), bcm_errmsg(rv));
                return CMD_FAIL;
            }
        } else {
            return CMD_USAGE;   
        }
        
        
    } else {
        return CMD_USAGE;   
    }
	return CMD_OK;
}
