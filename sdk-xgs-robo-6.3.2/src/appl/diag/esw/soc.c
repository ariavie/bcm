/*
 * $Id: soc.c 1.35 Broadcom SDK $
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
 * SOC-related CLI commands
 */

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/dport.h>

#include <bcm/error.h>
#include <bcm/init.h>

#include <soc/mem.h>

#ifdef BCM_XGS12_FABRIC_SUPPORT
#include <appl/diag/pp.h>
#include <soc/hercules.h>
#endif

#ifdef BCM_XGS_SUPPORT
#include <soc/hash.h>
#endif

/*
 * Raw SOC reset
 */

char socres_usage[] =
    "Parameters: none\n\t"
    "Performs an SCP reset -- NOT recommended\n";

cmd_result_t
cmd_socres(int unit, args_t *a)
{
    if (!sh_check_attached(ARG_CMD(a), unit))
	return CMD_FAIL;

    if (soc_reset(unit) < 0)
	return CMD_FAIL;

    return CMD_OK;
}

/*
 * Set number of microseconds before timing out on the S-Channel DONE bit
 * during S-Channel operations.
 */

char stimeout_usage[] =
    "Parameters: [usec]\n\t"
    "Sets number of microseconds before S-Channel operations time out.\n\t"
    "If argument is not given, displays the current setting.\n";

cmd_result_t
cmd_stimeout(int unit, args_t *a)
{
    char *usec_str = ARG_GET(a);
    int usec;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (usec_str) {
	usec = parse_integer(usec_str);
	SOC_CONTROL(unit)->schanTimeout = usec;
    } else {
	printk("S-Channel timeout is %d usec\n",
	       SOC_CONTROL(unit)->schanTimeout);
    }

    return CMD_OK;
}

/*
 * Set number of microseconds before timing out on the MIIM DONE bit
 * during PHY register operations.
 */

char mtimeout_usage[] =
    "Parameters: [usec]\n\t"
    "Sets number of microseconds before MIIM operations time out.\n\t"
    "If argument is not given, displays the current setting.\n";

cmd_result_t
cmd_mtimeout(int unit, args_t *a)
{
    char *usec_str = ARG_GET(a);
    int usec;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (usec_str) {
	usec = parse_integer(usec_str);
	SOC_CONTROL(unit)->miimTimeout = usec;
    } else {
	printk("MIIM timeout is %d usec\n",
	       SOC_CONTROL(unit)->miimTimeout);
    }

    return CMD_OK;
}

/*
 * Set number of milliseconds before timing out on BIST operations.
 */

char btimeout_usage[] =
    "Parameters: [msec]\n\t"
    "Sets number of milliseconds before BIST operations time out.\n\t"
    "If argument is not given, displays the current setting.\n";

cmd_result_t
cmd_btimeout(int unit, args_t *a)
{
    char *msec_str = ARG_GET(a);
    int msec;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (msec_str) {
	msec = parse_integer(msec_str);
	SOC_CONTROL(unit)->bistTimeout = msec;
    } else {
	printk("BIST timeout is %d msec\n",
	       SOC_CONTROL(unit)->bistTimeout);
    }

    return CMD_OK;
}

/*
 * Perform generic S-channel operation given raw S-channel buffer words.
 */

char schan_usage[] =
    "Parameters: DW0 [... DWn]\n\t"
    "Sends the specified raw data as an S-channel message, waits for a\n\t"
    "response, then displays the S-channel message buffer (response).\n";

cmd_result_t
cmd_schan(int unit, args_t *a)
{
    schan_msg_t msg;
    int i;
    char *datastr = ARG_GET(a);
    int rv = CMD_FAIL;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	goto done;
    }

    if (datastr == NULL) {
	return CMD_USAGE;
    }

    sal_memset(&msg, 0, sizeof (msg));	/* Clear all words, not just header */

    for (i = 0; datastr; i++) {
	msg.dwords[i] = parse_integer(datastr);
	datastr = ARG_GET(a);
    }

    if ((i = soc_schan_op(unit, &msg,
			  CMIC_SCHAN_WORDS(unit),
			  CMIC_SCHAN_WORDS(unit), 1)) < 0) {
	printk("S-Channel operation failed: %s\n", soc_errmsg(i));
	goto done;
    }

    for (i = 0; i < CMIC_SCHAN_WORDS(unit); i++) {
	printk("0x%x ", msg.dwords[i]);
    }

    printk("\n");

    rv = CMD_OK;

 done:
    return rv;
}

cmd_result_t
cmd_esw_soc(int u, args_t *a)
/*
 * Function: 	sh_soc
 * Purpose:	Print soc control information if compiled in debug mode.
 * Parameters:	u - unit #
 *		a - pointer to args, expects <unit> ...., if none passed,
 *			default unit # used.
 * Returns:	CMD_OK/CMD_FAIL
 */
{
#if defined(BROADCOM_DEBUG)
    char 	*c;
    cmd_result_t rv = CMD_OK;

    if (!sh_check_attached("soc", u)) {
	return(CMD_FAIL);
    }

    if (!ARG_CNT(a)) {
	rv = soc_dump(u, "  ") ? CMD_FAIL : CMD_OK;
    } else {
	while ((CMD_OK == rv) && (c = ARG_GET(a))) {
	    if (!isint(c)) {
		printk("%s: Invalid unit identifier: %s\n", ARG_CMD(a), c);
		rv = CMD_FAIL;
	    } else {
		rv = soc_dump(parse_integer(c), "  ") ?
		    CMD_FAIL : CMD_OK;
	    }
	}
    }
    return(rv);
#else /* !defined(BROADCOM_DEBUG) */
    printk("%s: Warning: Not compiled with BROADCOM_DEBUG enabled\n", ARG_CMD(a));
    return(CMD_OK);
#endif /* defined(BROADCOM_DEBUG) */
}

/*
 * Utility routine for BIST command as well as BIST diagnostic
 */

static void
bist_add_mem(soc_mem_t *mems, int *num_mems, soc_mem_t mem)
{
    int 		i;

    for (i = 0; i < *num_mems; i++) {
	if (mems[i] == mem) {
	    return;		/* Already there */
	}
    }

    mems[(*num_mems)++] = mem;
}

int
bist_args_to_mems(int unit, args_t *a, soc_mem_t *mems, int *num_mems)
{
    char		*parm;
    soc_mem_t		mem;
    int			rv = 0;

    *num_mems = 0;

    while ((parm = ARG_GET(a)) != 0) {
	if (sal_strcasecmp(parm, "all") == 0) {
	    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
		if (soc_mem_is_valid(unit, mem) &&
		    (soc_mem_is_bistepic(unit, mem) ||
		     soc_mem_is_bistcbp(unit, mem) ||
		     soc_mem_is_bistffp(unit, mem))) {
		    bist_add_mem(mems, num_mems, mem);
		}
	    }
	} else if (sal_strcasecmp(parm, "cbp") == 0) {
	    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
		if (soc_mem_is_valid(unit, mem) &&
		    soc_mem_is_cbp(unit, mem)) {
		    bist_add_mem(mems, num_mems, mem);
		}
	    }
	} else if (parse_memory_name(unit, &mem, parm, 0, 0) < 0) {
	    printk("BIST ERROR: Unknown memory name: %s\n", parm);
	    rv = -1;
	} else {
	    bist_add_mem(mems, num_mems, mem);
	}
    }

    if (*num_mems == 0) {
	printk("BIST ERROR: No memories specified\n");
	rv = -1;
    }

    return rv;
}

/*
 * BIST (non-diagnostic version)
 */

char bist_usage[] =
    "Parameters: [table] ...\n\t"
    "Runs raw BIST (built-in self-test) on list of BISTable memories.\n\t"
    "Use \"listmem\" to get a list of BISTable memories.\n\t"
    "The special name CBP can be used to refer to all CBP memories.\n";

cmd_result_t
cmd_bist(int unit, args_t *a)
{
    soc_mem_t		*mems;
    int 		num_mems;
    int			rv;

    /*BIST is not supported on XGSIII devices*/
    UNSUPPORTED_COMMAND(unit, SOC_CHIP_BCM56601, a);
    UNSUPPORTED_COMMAND(unit, SOC_CHIP_BCM56602, a);
    UNSUPPORTED_COMMAND(unit, SOC_CHIP_BCM56504, a);
    UNSUPPORTED_COMMAND(unit, SOC_CHIP_BCM56102, a);
    UNSUPPORTED_COMMAND(unit, SOC_CHIP_BCM56304, a);
    UNSUPPORTED_COMMAND(unit, SOC_CHIP_BCM56112, a);
    UNSUPPORTED_COMMAND(unit, SOC_CHIP_BCM56314, a);
    UNSUPPORTED_COMMAND(unit, SOC_CHIP_BCM56514, a);
    UNSUPPORTED_COMMAND(unit, SOC_CHIP_BCM5650, a);

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    mems = sal_alloc(sizeof(soc_mem_t) * NUM_SOC_MEM,
                                  "BIST mem list");
    if (NULL == mems) {
	printk("Insufficient memory for BIST\n");
        return BCM_E_MEMORY;
    }

    if (bist_args_to_mems(unit, a, mems, &num_mems) < 0) {
        sal_free(mems);
	return CMD_FAIL;
    }

    if ((rv = soc_bist(unit,
		       mems, num_mems,
		       SOC_CONTROL(unit)->bistTimeout)) < 0) {
	printk("BIST failed: %s\n", soc_errmsg(rv));
        sal_free(mems);
	return CMD_FAIL;
    }

    sal_free(mems);
    return CMD_OK;
}

/*
 * PBMP
 */
cmd_result_t
cmd_esw_bcm_pbmp(int unit, args_t *a)
{
    pbmp_t		pbmp;
    pbmp_t		st;
    char		*c;
    bcm_port_t		port;
    bcm_port_config_t   pcfg;
    char		pbmp_str[FORMAT_PBMP_MAX];
    char		pfmt[SOC_PBMP_FMT_LEN];

    COMPILER_REFERENCE(unit);

    BCM_IF_ERROR_RETURN(bcm_port_config_get(unit, &pcfg));

    c = ARG_GET(a);

    if (!c) {
        BCM_PBMP_ASSIGN(st, pcfg.stack_ext);
        BCM_PBMP_OR(st, pcfg.stack_int);

        printk("Current BCM bitmaps:\n");
        printk("     FE   ==> %s\n",
               SOC_PBMP_FMT(pcfg.fe, pfmt));
        printk("     GE   ==> %s\n",
               SOC_PBMP_FMT(pcfg.ge, pfmt));
        printk("     XE   ==> %s\n",
               SOC_PBMP_FMT(pcfg.xe, pfmt));
        printk("     E    ==> %s\n",
               SOC_PBMP_FMT(pcfg.e, pfmt));
        printk("     HG   ==> %s\n",
               SOC_PBMP_FMT(pcfg.hg, pfmt));
        printk("     HL   ==> %s\n",
               SOC_PBMP_FMT(PBMP_HL_ALL(unit), pfmt));
        printk("     ST   ==> %s\n",
               SOC_PBMP_FMT(st, pfmt));
        printk("     PORT ==> %s\n",
               SOC_PBMP_FMT(pcfg.port, pfmt));
        printk("     CMIC ==> %s\n",
               SOC_PBMP_FMT(pcfg.cpu, pfmt));
        printk("     ALL  ==> %s\n",
               SOC_PBMP_FMT(pcfg.all, pfmt));
        return CMD_OK;
    }

    if (sal_strcasecmp(c, "port") == 0) {
        if ((c = ARG_GET(a)) == NULL) {
            printk("ERROR: missing port string\n");
            return CMD_FAIL;
        }
        if (parse_bcm_port(unit, c, &port) < 0) {
            printk("%s: Invalid port string: %s\n", ARG_CMD(a), c);
            return CMD_FAIL;
        }
        printk("    port %s ==> %s (%d)\n",
               c, BCM_PORT_NAME(unit, port), port);
        return CMD_OK;
    }

    if (parse_bcm_pbmp(unit, c, &pbmp) < 0) {
        printk("%s: Invalid pbmp string (%s); use 'pbmp ?' for more info.\n",
               ARG_CMD(a), c);
        return CMD_FAIL;
    }

    format_bcm_pbmp(unit, pbmp_str, sizeof (pbmp_str), pbmp);

    printk("    %s ==> %s\n", SOC_PBMP_FMT(pbmp, pfmt), pbmp_str);

    return CMD_OK;
}

cmd_result_t
cmd_esw_pbmp(int unit, args_t *a)
{
    pbmp_t		pbmp;
    char		*c;
    soc_port_t		port;
    char		pbmp_str[FORMAT_PBMP_MAX];
    char		pfmt[SOC_PBMP_FMT_LEN];

    COMPILER_REFERENCE(unit);

    c = ARG_GET(a);

    if (!c) {
        printk("Current bitmaps:\n");
        printk("     FE   ==> %s\n",
               SOC_PBMP_FMT(PBMP_FE_ALL(unit), pfmt));
        printk("     GE   ==> %s\n",
               SOC_PBMP_FMT(PBMP_GE_ALL(unit), pfmt));
        printk("     XE   ==> %s\n",
               SOC_PBMP_FMT(PBMP_XE_ALL(unit), pfmt));
#ifdef BCM_CMAC_SUPPORT
        printk("     CE   ==> %s\n",
               SOC_PBMP_FMT(PBMP_CE_ALL(unit), pfmt));
#endif
        printk("     E    ==> %s\n",
               SOC_PBMP_FMT(PBMP_E_ALL(unit), pfmt));
        printk("     HG   ==> %s\n",
               SOC_PBMP_FMT(PBMP_HG_ALL(unit), pfmt));
        printk("     LP   ==> %s\n",
               SOC_PBMP_FMT(PBMP_LP_ALL(unit), pfmt));
        printk("     IL   ==> %s\n",
               SOC_PBMP_FMT(PBMP_IL_ALL(unit), pfmt));
        printk("     SCH  ==> %s\n",
               SOC_PBMP_FMT(PBMP_SCH_ALL(unit), pfmt));
        printk("     HL   ==> %s\n",
               SOC_PBMP_FMT(PBMP_HL_ALL(unit), pfmt));
        printk("     HG2  ==> %s\n",
               SOC_PBMP_FMT(SOC_HG2_PBM(unit), pfmt));
        printk("     ST   ==> %s\n",
               SOC_PBMP_FMT(PBMP_ST_ALL(unit), pfmt));
        printk("     S    ==> %s\n",
               SOC_PBMP_FMT(SOC_INFO(unit).s_pbm, pfmt));
        printk("     GX   ==> %s\n",
               SOC_PBMP_FMT(PBMP_GX_ALL(unit), pfmt));
        printk("     XL   ==> %s\n",
               SOC_PBMP_FMT(PBMP_XL_ALL(unit), pfmt));
        printk("     MXQ  ==> %s\n",
               SOC_PBMP_FMT(PBMP_MXQ_ALL(unit), pfmt));
        printk("     XG   ==> %s\n",
               SOC_PBMP_FMT(PBMP_XG_ALL(unit), pfmt));
        printk("     XQ   ==> %s\n",
               SOC_PBMP_FMT(PBMP_XQ_ALL(unit), pfmt));
        printk("     XT   ==> %s\n",
               SOC_PBMP_FMT(PBMP_XT_ALL(unit), pfmt));
        printk("     XW   ==> %s\n",
               SOC_PBMP_FMT(PBMP_XW_ALL(unit), pfmt));
        printk("     CL   ==> %s\n",
               SOC_PBMP_FMT(PBMP_CL_ALL(unit), pfmt));
        printk("     C    ==> %s\n",
               SOC_PBMP_FMT(PBMP_C_ALL(unit), pfmt));
        printk("     AXP  ==> %s\n",
               SOC_PBMP_FMT(PBMP_AXP_ALL(unit), pfmt));
        printk("     HPLT ==> %s\n",
               SOC_PBMP_FMT(PBMP_HYPLITE_ALL(unit), pfmt));
        printk("     PORT ==> %s\n",
               SOC_PBMP_FMT(PBMP_PORT_ALL(unit), pfmt));
        printk("     CMIC ==> %s\n",
               SOC_PBMP_FMT(PBMP_CMIC(unit), pfmt));
        printk("     LB   ==> %s\n",
               SOC_PBMP_FMT(PBMP_LB(unit), pfmt));
        printk("     MMU  ==> %s\n",
               SOC_PBMP_FMT(PBMP_MMU(unit), pfmt));
        printk("     ALL  ==> %s\n",
               SOC_PBMP_FMT(PBMP_ALL(unit), pfmt));
        return CMD_OK;
    }

    if (sal_strcasecmp(c, "bcm") == 0) {
        return cmd_esw_bcm_pbmp(unit, a);
    }

    if (sal_strcasecmp(c, "port") == 0) {
	if ((c = ARG_GET(a)) == NULL) {
	    printk("ERROR: missing port string\n");
	    return CMD_FAIL;
	}
	if (parse_port(unit, c, &port) < 0) {
	    printk("%s: Invalid port string: %s\n", ARG_CMD(a), c);
	    return CMD_FAIL;
	}
	printk("    port %s ==> %s (%d)\n",
	       c, SOC_PORT_NAME(unit, port), port);
	return CMD_OK;
    }

    if (parse_pbmp(unit, c, &pbmp) < 0) {
	printk("%s: Invalid pbmp string (%s); use 'pbmp ?' for more info.\n",
	       ARG_CMD(a), c);
	return CMD_FAIL;
    }

    format_pbmp(unit, pbmp_str, sizeof (pbmp_str), pbmp);

    printk("    %s ==> %s\n", SOC_PBMP_FMT(pbmp, pfmt), pbmp_str);

    return CMD_OK;
}

#ifdef BCM_XGS12_FABRIC_SUPPORT

char if_xqdump_usage[] =
    "Usage :\n\t"
    "  xqdump <pbmp> [cos] [xqptr]\n\t"
    "          - Display packets pending in the XQ\n\t"
    "          - No COS selects all COS\n\t"
    "          - xqptr will try to dump packets already transmitted\n\t"
    "          - NOTE! This command will probably trash the chip state!\n";

cmd_result_t
if_xqdump(int unit, args_t * args)
{
    char                *argpbm, *argcos, *argptr;
    pbmp_t              pbm;
    int                 rv;
    int		        cos_start, cos_end, xq_ptr;
    soc_port_t          port, dport;

    if (!SOC_IS_XGS12_FABRIC(unit)) {
        printk("Command only valid for BCM5670/75 fabric\n");
        return CMD_FAIL;
    }

    if ((argpbm = ARG_GET(args)) == NULL) {
        pbm = PBMP_PORT_ALL(unit);
    } else {
        if (parse_pbmp(unit, argpbm, &pbm) < 0) {
            printk("%s: Error: unrecognized port bitmap: %s\n",
                    ARG_CMD(args), argpbm);
            return CMD_FAIL;
        }
    }

    if ((argcos = ARG_GET(args)) == NULL) {
        cos_start = 0;
        cos_end = NUM_COS(unit) - 1;
    } else {
        cos_start = cos_end = sal_ctoi(argcos, 0);

        if (cos_start >= NUM_COS(unit)) {
            printk("%s: Error: COS out of range: %s\n",
                    ARG_CMD(args), argcos);
            return CMD_FAIL;
        }
    }

    if ((argptr = ARG_GET(args)) == NULL) {
        DPORT_SOC_PBMP_ITER(unit, pbm, dport, port) {
            if ((rv = diag_dump_cos_packets(unit, port,
                                            cos_start, cos_end)) < 0) {
                printk("Error dumping packets for port %d: %s\n",
                       port, soc_errmsg(rv));
                return CMD_FAIL;
            }
        }
    } else {
        xq_ptr = sal_ctoi(argptr, 0);
        DPORT_SOC_PBMP_ITER(unit, pbm, dport, port) {
            diag_dump_xq_packet(unit, SOC_PORT_BLOCK(unit, port), xq_ptr);
        }
    }

    printk("XQ state probably trashed.  Recommend resetting the chip.\n");

    return CMD_OK;
}

char if_ibdump_usage[] =
    "Usage :\n\t"
    "  ibdump <pbmp>\n\t"
    "          - Display packets pending in the Ingress Buffer\n";

cmd_result_t
if_ibdump(int unit, args_t * args)
{
    char                *argpbm;
    pbmp_t              pbm;
    int                 rv;
    soc_port_t          port;

    if (!SOC_IS_XGS12_FABRIC(unit)) {
        printk("Command only valid for BCM5670/75 fabric\n");
        return CMD_FAIL;
    }

    if (!soc_feature(unit, soc_feature_fabric_debug)) {
        printk("Command not valid for BCM5670_A0\n");
        return CMD_FAIL;
    }

    if ((argpbm = ARG_GET(args)) == NULL) {
        pbm = PBMP_PORT_ALL(unit);
    } else {
        if (parse_pbmp(unit, argpbm, &pbm) < 0) {
            printk("%s: Error: unrecognized port bitmap: %s\n",
                    ARG_CMD(args), argpbm);
            return CMD_FAIL;
        }
    }

    PBMP_PORT_ITER(unit, port) {
        if (PBMP_MEMBER((pbm), port)) {
            if ((rv = diag_dump_ib_packets(unit, port)) < 0) {
                printk("Error dumping packets for port %d: %s\n",
		       port, soc_errmsg(rv));
                return CMD_FAIL;
            }
        }
    }

    return CMD_OK;
}

char if_xqerr_usage[] =
    "Parameters: [PortBitMap=<pbmp>] [COS=<cos>]\n\t"
    "[BitsError=<num>][PacketNum=<num>][BitNum=<bit>]\n\t"
    "          - Introduce bit-errors to packets pending in the XQ\n\t"
    "          - Only one COS should be configured for this port,\n\t"
    "            else the chip state will be hopelessly scrambled.\n";

cmd_result_t
if_xqerr(int unit, args_t * args)
{
    parse_table_t	pt;
    soc_pbmp_t          pbm;
    int                 rv;
    int		        cos, errors, packet, bit;
    soc_port_t          port, dport;

    if (!SOC_IS_XGS12_FABRIC(unit)) {
        printk("Command only valid for BCM5670/75 fabric\n");
        return CMD_FAIL;
    }

    SOC_PBMP_CLEAR(pbm);
    cos = 0;
    errors = 1;
    packet = 0;
    bit = 0;

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "PortBitMap", PQ_PBMP | PQ_DFL, 0, &pbm, 0);
    parse_table_add(&pt, "COS", PQ_INT | PQ_DFL, 0, &cos, 0);
    parse_table_add(&pt, "BitsError", PQ_INT | PQ_DFL, 0, &errors, 0);
    parse_table_add(&pt, "PacketNum", PQ_INT | PQ_DFL, 0, &packet, 0);
    parse_table_add(&pt, "BitNum", PQ_INT | PQ_DFL, 0, &bit, 0);

    if (parse_arg_eq(args, &pt) < 0) {
	printk("%s: Invalid argument: %s\n", ARG_CMD(args), ARG_CUR(args));
	parse_arg_eq_done(&pt);
	return(CMD_FAIL);
    }
    parse_arg_eq_done(&pt);

    if ( cos >= NUM_COS(unit) ) {
        printk("%s: Error: COS out of range: %d\n",
               ARG_CMD(args), cos);
            return CMD_FAIL;
    }

    if ( errors > 2 ) {
        printk("%s: Error: Excessive bit errors selected: %d\n",
               ARG_CMD(args), errors);
    }

    DPORT_SOC_PBMP_ITER(unit, pbm, dport, port) {
        if ((rv = diag_set_cos_errors(unit, port, cos,
                                     errors, packet, bit)) < 0) {
            printk("Error adjusting XQ packets for port %d: %s\n",
                   port, soc_errmsg(rv));
            return CMD_FAIL;
        }
    }

    return CMD_OK;
}

char if_mmu_cfg_usage[] =
    "Usage :\n\t"
    "Parameters: [PortBitMap=<pbmp>] [COSNum=<cos>]\n\t"
    "[LosslessMode=true|false]\n\t"
    "          - Configure MMU for the given ports and number of COS.\n\t"
    "          - Lossless mode is ingress throttle.\n\t"
    "          - not Lossless is throughput mode or egress throttle.\n";

cmd_result_t
if_mmu_cfg(int unit, args_t * args)
{
    parse_table_t       pt;
    pbmp_t              pbm;
    int                 rv;
    int                 num_ports, num_cos, lossless;
    soc_port_t          port, dport;

    if (!SOC_IS_XGS12_FABRIC(unit)) {
        printk("Command only valid for BCM5670/75 fabric\n");
        return CMD_FAIL;
    }

    pbm = PBMP_ALL(unit);
    num_cos = 4;     /* Safe for both modes */
    lossless = FALSE;

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "PortBitMap", PQ_PBMP | PQ_DFL, 0, &pbm, 0);
    parse_table_add(&pt, "COSNum", PQ_INT | PQ_DFL, 0, &num_cos, 0);
    parse_table_add(&pt, "LosslessMode", PQ_BOOL | PQ_DFL, 0, &lossless, 0);

    if (parse_arg_eq(args, &pt) < 0) {
	printk("%s: Invalid argument: %s\n", ARG_CMD(args), ARG_CUR(args));
	parse_arg_eq_done(&pt);
	return(CMD_FAIL);
    }
    parse_arg_eq_done(&pt);

    if (num_cos < 1) {
        printk("No COS selected.\n");
        return CMD_FAIL;
    } else if (num_cos > NUM_COS(unit)) {
        printk("Too many COS selected, unit only allows %d.\n",
               NUM_COS(unit));
        return CMD_FAIL;
    }

    if (lossless && (num_cos > 4)) {
	printk("%s: %d COS selected, lossless mode allows only 4.\n",
               ARG_CMD(args), num_cos);
	return(CMD_FAIL);
    }

    /* Only "real" ports */
    SOC_PBMP_AND(pbm, PBMP_ALL(unit));

    /* Count the ports */
    num_ports = 0;
    SOC_PBMP_ITER(pbm, port) {
        num_ports++;
    }

    if (num_ports < 1) {
        printk("No ports selected.\n");
        return CMD_FAIL;
    }

    /* Only want "other" port count, but don't try 0. */
    if (num_ports > 1) {
        num_ports--;
    }

    if (SOC_IS_HERCULES1(unit)) {
        DPORT_SOC_PBMP_ITER(unit, pbm, dport, port) {
            if ((rv = soc_hercules_mmu_limits_config(unit, port,
                                    num_ports, num_cos, lossless)) < 0) {
                printk("Error configuring MMU for port %d: %s\n",
                       port, soc_errmsg(rv));
                return CMD_FAIL;
            }
        }
    } else {
        DPORT_SOC_PBMP_ITER(unit, pbm, dport, port) {
            if ((rv = soc_hercules15_mmu_limits_config(unit, port,
                                    num_ports, num_cos, lossless)) < 0) {
                printk("Error configuring MMU for port %d: %s\n",
                       port, soc_errmsg(rv));
                return CMD_FAIL;
            }
        }
    }

    return CMD_OK;
}

#endif /* BCM_XGS12_FABRIC_SUPPORT */

#ifdef BCM_XGS_SWITCH_SUPPORT

/*
 * Hash Select
 *
 *   Currently the only thing handled by this routine is HashSelect for
 *   Draco, however it is designed so it can be extended to support any
 *   other hash settings for any other chips.  Any such things should be
 *   added to the parse table below.
 */

char hash_usage[] =
    "Parameters: [HashSelect=<U16|L16|LSB|ZERO|U32|L32>]\n\t"
    "Displays all hash parameters if no arguments given.\n\t"
    "Note: if HashSelect is changed, L2 and L3 tables should be cleared.\n";

cmd_result_t
cmd_hash(int unit, args_t *a)
{
    parse_table_t	pt;
    int			r;
    int			hash_sel;
    char		**hashSels;
    static char		*xgs2hashSels[] = {
	"u16", "l16", "lsb", "zero", "u32", "l32", NULL
    };
    static char		*xgs3hashSels[] = {
	"zero", "u32", "l32", "lsb", "l16", "u16", NULL
    };

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if (!soc_feature(unit, soc_feature_arl_hashed)) {
	printk("%s: No hash features on this chip\n", ARG_CMD(a));
	return CMD_FAIL;
    }

    if (SOC_IS_XGS3_SWITCH(unit)) {
	hashSels = xgs3hashSels;
    } else if (SOC_IS_XGS12_SWITCH(unit)) {
	hashSels = xgs2hashSels;
    } else {
	printk("%s: No hash features on this chip\n", ARG_CMD(a));
	return CMD_FAIL;
    }

    r = SOC_E_UNAVAIL;
#ifdef	BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
	uint32	hreg;
 	r = READ_HASH_CONTROLr(unit, &hreg);
	hash_sel = soc_reg_field_get(unit, HASH_CONTROLr,
				     hreg, L2_AND_VLAN_MAC_HASH_SELECTf);
    }
#endif
#ifdef	BCM_XGS12_SWITCH_SUPPORT
    if (SOC_IS_XGS12_SWITCH(unit)) {
	r = soc_draco_hash_get(unit, &hash_sel);
    }
#endif

    if (r < 0) {
	printk("%s: Error getting hash select: %s\n",
	       ARG_CMD(a), soc_errmsg(r));
	return CMD_FAIL;
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "HashSelect", PQ_DFL|PQ_MULTI,
		    (void *)0, &hash_sel, hashSels);

    if (!ARG_CNT(a)) {			/* Display settings */
	printk("Current settings:\n");
	printk("  HashSelect=%s\n", hashSels[hash_sel]);
	parse_arg_eq_done(&pt);
	return(CMD_OK);
    }

    if (parse_arg_eq(a, &pt) < 0) {
	printk("%s: Error: Unknown option: %s\n", ARG_CMD(a), ARG_CUR(a));
	parse_arg_eq_done(&pt);
	return(CMD_FAIL);
    }
    parse_arg_eq_done(&pt);

    r = SOC_E_UNAVAIL;
#ifdef	BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
	uint32	hreg;
 	r = READ_HASH_CONTROLr(unit, &hreg);
	soc_reg_field_set(unit, HASH_CONTROLr, &hreg,
			  L2_AND_VLAN_MAC_HASH_SELECTf, hash_sel);
	if (r >= 0) {
	    r = WRITE_HASH_CONTROLr(unit, hreg);
	}
    }
#endif
#ifdef	BCM_XGS12_SWITCH_SUPPORT
    if (SOC_IS_XGS12_SWITCH(unit)) {
	r = soc_draco_hash_set(unit, hash_sel);
    }
#endif
    if (r < 0) {
	printk("%s: Error setting hash select: %s\n",
	       ARG_CMD(a), soc_errmsg(r));
	return CMD_FAIL;
    }

    return CMD_OK;
}

#endif /* BCM_XGS_SWITCH_SUPPORT */

/****************************************************************
 * Interrupt enable/disable commands.
 ****************************************************************/
static struct {
    soc_field_t field;
    char *name;
} irq_info[] = { /* array of all possible fields of all supported devices */
    { BSAFE_OP_DONEf,           "BsafeOpDone" },
    { BSE_CMDMEM_DONEf,         "BseCmdDone" },
    { CH0_CHAIN_DONEf,          "ChainDone0" },
    { CH0_DESC_DONEf,           "DescDone0" },
    { CH1_CHAIN_DONEf,          "ChainDone1" },
    { CH1_DESC_DONEf,           "DescDone1" },
    { CH2_CHAIN_DONEf,          "ChainDone2" },
    { CH2_DESC_DONEf,           "DescDone2" },
    { CH3_CHAIN_DONEf,          "ChainDone3" },
    { CH3_DESC_DONEf,           "DescDone3" },
    { CHIP_FUNC_INTR_0f,        "ChipFuncIntr0" },
    { CHIP_FUNC_INTR_1f,        "ChipFuncIntr1" },
    { CHIP_FUNC_INTR_2f,        "ChipFuncIntr2" },
    { CHIP_FUNC_INTR_3f,        "ChipFuncIntr3" },
    { CHIP_FUNC_INTR_4f,        "ChipFuncIntr4" },
    { CHIP_FUNC_INTR_5f,        "ChipFuncIntr5" },
    { CHIP_FUNC_INTR_6f,        "ChipFuncIntr6" },
    { CHIP_FUNC_INTR_7f,        "ChipFuncIntr7" },
    { CSE_CMDMEM_DONEf,         "CseCmdDone" },
    { EP_INTRf,                 "EpIntr" },
    { FIFO_CH0_DMA_INTRf,       "FifoDmaIntr0" },
    { FIFO_CH1_DMA_INTRf,       "FifoDmaIntr1" },
    { FIFO_CH2_DMA_INTRf,       "FifoDmaIntr2" },
    { FIFO_CH3_DMA_INTRf,       "FifoDmaIntr3" },
    { HSE_CMDMEM_DONEf,         "HseCmdDone" },
    { I2C_INTRf,                "I2Cintr" },
    { IP_INTRf,                 "IpIntr" },
    { L2_MOD_FIFO_NOT_EMPTYf,   "L2ModFifoNotEmpty" },
    { LINK_STAT_MODf,           "LinkStatMod" },
    { MEM_FAILf,                "MemFail" },
    { MIIM_OP_DONEf,            "MiimOpDone" },
    { MMU_GROUP_INTRf,          "MmuGroupIntr" },
    { PCI_FATAL_ERRf,           "PciFatalErr" },
    { PCI_PARITY_ERRf,          "PciParityErr" },
    { RX_PAUSE_STAT_MODf,       "RxPauseStatMod" },
    { SCHAN_ERRf,               "SchanErr" },
    { SCH_MSG_DONEf,            "SchMsgDone" },
    { SLAM_DMA_COMPLETEf,       "SlamDmaComplete" },
    { STATS_DMA_ITER_DONEf,     "StatIterDone" },
    { STAT_DMA_DONEf,           "StatDmaDone" },
    { STATS_DMA_DONEf,          "StatDmaDone" },
    { TABLE_DMA_COMPLETEf,      "TableDmaCompete" },
    { TX_PAUSE_STAT_MODf,       "TxPauseStatMod" },
    { BROADSYNC_INTERRUPTf,     "Broadsync" }
};                

char cmd_intr_usage[] =
    "Usages:\n"
    "  INTR Enable  [<mask> | <irq_name> ...]   - Enable interrupts\n"
    "  INTR Disable [<mask> | <irq_name> ...]   - Disable interrupts\n"
    "  INTR Mask                                - Show current mask reg\n"
    "  INTR Pending                             - Show current state\n"
    "  INTR Names                               - List interrupt names\n";

static int parse_irq_list(args_t *a, char **irq_names, uint32 *irq_masks,
                          int count, uint32 *mask)
{
    char *intr;
    int i;

    if ((intr = ARG_GET(a)) == NULL) {
        return -1;
    }

    if ((*mask = sal_ctoi(intr, 0)) == 0) { /* Assume arg list */
        do {
            for (i = 0; i < count; i++) {
                if (parse_cmp(irq_names[i], intr, '\0')) {
                    break;
                }
            }
            if (i == count) {
                return -1;
            }
            *mask |= irq_masks[i];
        } while ((intr = ARG_GET(a)) != NULL);
    }
    return 0;
}

cmd_result_t
cmd_intr(int unit, args_t *a)
{
    char *subcmd;
    uint32  mask = 0;
    uint32  omask;
    int i, j, count;
    char *irq_names[33];
    uint32 irq_masks[33];
    soc_reg_info_t *reg_info;
    soc_field_info_t *field_info;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((subcmd = ARG_GET(a)) == NULL) {
	return CMD_USAGE;
    }

    reg_info = &SOC_REG_INFO(unit, CMIC_IRQ_STATr);
    mask = 0;
    count = 0;
    for (i = 0; i < reg_info->nFields; i++) {
        field_info = &reg_info->fields[reg_info->nFields - 1 - i];
        if (field_info->flags & SOCF_RES) {
            continue;
        }
        for (j = 0; j < sizeof(irq_info) / sizeof(irq_info[0]); j++) {
            if (irq_info[j].field == field_info->field) {
                irq_names[count] = irq_info[j].name;
                break;
            }
        }
        if (j == sizeof(irq_info) / sizeof(irq_info[0])) {
            printk("Could not parse %s (0x%08x)\n"
                   "It needs to be added to the list of interrupt names\n",
                   SOC_FIELD_NAME(unit, field_info->field),
                   1 << field_info->bp);
            irq_names[count] = SOC_FIELD_NAME(unit, field_info->field);
        }
        irq_masks[count] = 1 << field_info->bp;
        mask |= irq_masks[count];
        count++;
    }
    irq_names[count] = "ALL";
    irq_masks[count] = mask;
    count++;

    if (parse_cmp("Enable", subcmd, '\0')) {
        if (parse_irq_list(a, irq_names, irq_masks, count, &mask)) {
            printk("Could not parse list\n");
            return CMD_USAGE;
        }
        omask = soc_intr_enable(unit, mask);
        printk ("Enabled with mask 0x%08x.  "
                "Mask was 0x%08x\n", mask, omask);
    } else if (parse_cmp("Disable", subcmd, '\0')) {
        if (parse_irq_list(a, irq_names, irq_masks, count, &mask)) {
            printk("Could not parse list\n");
            return CMD_USAGE;
        }
        omask = soc_intr_disable(unit, mask);
        printk ("Disabled with mask 0x%08x.  "
                "Mask was 0x%08x\n", mask, omask);
    } else if (parse_cmp("Mask", subcmd, '\0')) {
        mask = soc_pci_read(unit, CMIC_IRQ_MASK);
        if (mask) {
            printk("Current interrupt control reg: 0x%08x\n", mask);
            printk("Interrupts enabled for the following:\n");
            for (i = 0; i < count - 1; i++) {
                if (mask & irq_masks[i]) {
                    printk("%-30s\t\n", irq_names[i]);
                    mask &= ~irq_masks[i];
                }
            }
            if (mask) {
                printk("Warning:  Unknown interrupts are enabled: 0x%08x\n",
                       mask);
            }
        } else {
            printk("No interrupts enabled\n");
        }
    } else if (parse_cmp("Pending", subcmd, '\0')) {
        mask = soc_pci_read(unit, CMIC_IRQ_STAT);
        if (mask) {
            printk("Current interrupt status reg: 0x%08x\n", mask);
            printk("The following interrupts are pending:\n");
            for (i = 0; i < count - 1; i++) {
                if (mask & irq_masks[i]) {
                    printk("%-30s\t\n", irq_names[i]);
                    mask &= ~irq_masks[i];
                }
            }
            if (mask) {
                printk("Warning:  Unknown interrupts are pending: 0x%08x\n",
                       mask);
            }
        } else {
            printk("No interrupts pending\n");
        }
    } else if (parse_cmp("Names", subcmd, '\0')) {
        printk("%-30s   %s\n", "Name", "Mask");
        for (i = 0; i < count; i++) {
            printk("%-30s0x%08x\n", irq_names[i], irq_masks[i]);
        }
    } else {
        return CMD_USAGE;
    }

    return CMD_OK;
}
