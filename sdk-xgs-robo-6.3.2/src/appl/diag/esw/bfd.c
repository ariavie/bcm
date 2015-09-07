/*
 * $Id: bfd.c 1.10 Broadcom SDK $
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

#if defined(INCLUDE_BFD)

#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#include <bcm/error.h>
#include <bcm/bfd.h>
#include <bcm_int/esw/port.h>
#if defined(BCM_ENDURO_SUPPORT)
#include <bcm/rx.h>
#include <soc/higig.h>
#endif

#define CLEAN_UP_AND_RETURN(_result) \
    parse_arg_eq_done(&parse_table); \
    return (_result);

#define _isprintable(_c) (((_c) > 32) && ((_c) < 127))

#define ENDPOINT_LIST_HEADER \
    "ID  Type  VRF  GPORT V6  IPADDR  DISCRIMINATOR  CpuPri  IntPri \n" \
    "--  ----  ---  ----- --  ------  -------------  ------  ------ \n"

char cmd_esw_bfd_usage[] = 
#ifdef COMPILER_STRING_CONST_LIMIT
    "Usage: bfd <option> [args...]\n"
#else
    "\n"
    "  bfd init\n"
    "  bfd endpoint add [GPORT=<id>] [TYPE=<type>] [VRF=<id>] [INTPRI=<intpri>]\n"
    "                   [CPUPRI=<qid>] [DISCRIMINATOR=<id>] [V6] [IP=<val>]\n"
    "  bfd endpoint update [ID=<id>] [TYPE=<type>] [VRF=<id>] [INTPRI=<intpri>]\n"
    "                       [CPUPRI=<qid>] [DISCRIMINATOR=<id>] [V6] [IP=<val>]\n"
    "  bfd endpoint delete [ID=<id>]\n"
    "  bfd endpoint show [ID=<id>] \n"
#if defined(BCM_KATANA_SUPPORT)
    "  bfd tx ID=<endpointid> \n"
#endif
#endif
    ;

cmd_result_t cmd_esw_bfd(int unit, args_t *args)
{
    char *arg_string_p = NULL;
    parse_table_t parse_table;
    bcm_bfd_endpoint_info_t endpoint_info;
    int int_pri;
    int result;
    int gport, type, vrf,qid, disc_id, v6, src_ip, dst_ip, label;
    
    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL)
    {
        return CMD_USAGE;
    }

    if (!sh_check_attached(ARG_CMD(args), unit))
    {
        return CMD_FAIL;
    }

    if (sal_strcasecmp(arg_string_p, "init") == 0)
    {
        result = bcm_bfd_init(unit);

        if (BCM_FAILURE(result))
        {
            printk("Command failed.  %s.\n", bcm_errmsg(result));

            return CMD_FAIL;
        }

        printk("BFD module initialized.\n");
    }
    
#if defined(BCM_KATANA_SUPPORT)
    else if (sal_strcasecmp(arg_string_p, "tx") == 0)
    {    
#if 0
        parse_table_init(unit, &parse_table);

        parse_table_add(&parse_table, "ID", PQ_INT,
            (void *) BCM_OAM_GROUP_INVALID, &endpoint_info.id, NULL);

        parse_table_add(&parse_table, "DMAC", PQ_MAC | PQ_DFL, 0,
                &mac_dst, NULL);

        parse_table_add(&parse_table, "SMAC", PQ_MAC | PQ_DFL, 0,
                &mac_src, NULL);

        if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
        {
            printk("Invalid option: %s\n", ARG_CUR(args));

            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        result = bcm_bfd_endpoint_get(unit, endpoint_info.id, &endpoint_info);
        
        if (BCM_FAILURE(result))
        {
            printk("Command failed.  %s.\n", bcm_errmsg(result));
        
            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }

        parse_arg_eq_done(&parse_table);
#endif
    }
#endif /* BCM_KATANA_SUPPORT */
    else if (sal_strcasecmp(arg_string_p, "endpoint") == 0)
    {
        arg_string_p = ARG_GET(args);

        bcm_bfd_endpoint_info_t_init(&endpoint_info);

        if (arg_string_p == NULL ||
            sal_strcasecmp(arg_string_p, "show") == 0)
        {
            parse_table_init(unit, &parse_table);
            
            parse_table_add(&parse_table, "ID", PQ_INT,
			(void *) BCM_BFD_ENDPOINT_INVALID, &endpoint_info.id, NULL);

            if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
            {
                printk("Invalid option: %s\n", ARG_CUR(args));

                CLEAN_UP_AND_RETURN(CMD_FAIL);
            }
            
            result = bcm_bfd_endpoint_get(unit, endpoint_info.id, &endpoint_info);
            if (BCM_FAILURE(result))
            {
                printk("Command failed. %s\n", bcm_errmsg(result));

                return CMD_FAIL;
            }

	    printk(ENDPOINT_LIST_HEADER);

	    printk("%1d  ", endpoint_info.id);
	    printk("%4d  ", endpoint_info.type);
            printk("%4d  ", endpoint_info.vrf_id);
	    printk("%4d  ", endpoint_info.gport);
	    printk("%2d  ",  (endpoint_info.flags & BCM_BFD_ENDPOINT_IPV6)>>13);
	    printk("0x%x ",  endpoint_info.src_ip_addr);
	    printk("0x%x ",  endpoint_info.dst_ip_addr);
	    printk("%10d ",  endpoint_info.local_discr);
	    printk("%12d ",  endpoint_info.cpu_qid);
	    printk("%6d \n",  endpoint_info.int_pri);

            return CMD_OK;            

        }
        else if (sal_strcasecmp(arg_string_p, "delete") == 0)
        {
            parse_table_init(unit, &parse_table);

            parse_table_add(&parse_table, "ID", PQ_INT,
                (void *) BCM_BFD_ENDPOINT_INVALID, &endpoint_info.id, NULL);

            if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
            {
                printk("Invalid option: %s\n", ARG_CUR(args));

                CLEAN_UP_AND_RETURN(CMD_FAIL);
            }

            result = bcm_bfd_endpoint_destroy(unit, endpoint_info.id);
            
            if (BCM_FAILURE(result))
            {
                printk("Command failed. %s\n", bcm_errmsg(result));

                return CMD_FAIL;
            }

            return CMD_OK;
        }
        else
        {
            bcm_bfd_endpoint_info_t_init(&endpoint_info);

            if (sal_strcasecmp(arg_string_p, "update") == 0)
            {
                endpoint_info.flags |= BCM_BFD_ENDPOINT_UPDATE;
            }
            else if (sal_strcasecmp(arg_string_p, "add") != 0)
            {
                printk("Invalid BFD endpoint command: %s\n", arg_string_p);

                return CMD_FAIL;
            }
          
            parse_table_init(unit, &parse_table);

            parse_table_add(&parse_table, "ID", PQ_INT,
               (void *) BCM_BFD_ENDPOINT_INVALID, &endpoint_info.id, NULL);

            parse_table_add(&parse_table, "GPORT", PQ_INT,
                0, &gport, NULL);
            parse_table_add(&parse_table, "TYPE", PQ_INT,
                0, &type, NULL);
            parse_table_add(&parse_table, "VRF", PQ_INT,
                0, &vrf, NULL);
            parse_table_add(&parse_table, "INTPRI", PQ_INT, 
                0, &int_pri, NULL);
            parse_table_add(&parse_table, "CPUPRI", PQ_INT, 
                0, &qid, NULL);
            parse_table_add(&parse_table, "DISCRIMINATOR", PQ_INT, 
                0, &disc_id, NULL);
            parse_table_add(&parse_table, "V6", PQ_INT, 
                0, &v6, NULL);
            parse_table_add(&parse_table, "SIP", PQ_INT, 
                0, &src_ip, NULL);            
            parse_table_add(&parse_table, "DIP", PQ_INT, 
                0, &dst_ip, NULL);            
            parse_table_add(&parse_table, "LABEL", PQ_INT, 
                0, &label, NULL);            

            if (parse_arg_eq(args, &parse_table) < 0 || ARG_CNT(args) > 0)
            {
                printk("Invalid option: %s\n", ARG_CUR(args));

                CLEAN_UP_AND_RETURN(CMD_FAIL);
            }

            if (endpoint_info.id != BCM_BFD_ENDPOINT_INVALID)
            {
                endpoint_info.flags |= BCM_BFD_ENDPOINT_WITH_ID;
            }

            if (v6) {
                endpoint_info.flags |= BCM_BFD_ENDPOINT_IPV6;
            }

            if (int_pri < 0)
            {
                 printk("An internal priority is required.\n");

                 CLEAN_UP_AND_RETURN(CMD_FAIL);
            }

            endpoint_info.gport = BCM_GPORT_LOCAL_SET(gport, gport);
            endpoint_info.type = type;
            endpoint_info.vrf_id = vrf;
            endpoint_info.local_discr = disc_id;
            endpoint_info.cpu_qid = qid;
            endpoint_info.int_pri = int_pri;
            endpoint_info.label = label;
            endpoint_info.src_ip_addr = src_ip;
            endpoint_info.dst_ip_addr = dst_ip;

            result = bcm_bfd_endpoint_create(unit, &endpoint_info);

            if (BCM_FAILURE(result))
            {
                printk("Command failed. %s\n", bcm_errmsg(result));

                parse_arg_eq_done(&parse_table);

                return CMD_FAIL;
            }

            parse_arg_eq_done(&parse_table);
            
            printk("BFD endpoint %d created.\n", endpoint_info.id);
          }

        }
    else
    {
        printk("Invalid BFD subcommand: %s\n", arg_string_p);

        return CMD_FAIL;
    }

    return CMD_OK;
}

#else
    int  __no_complilation_complaints_about_bfd__1;
#endif /* INCLUDE_BFD */
