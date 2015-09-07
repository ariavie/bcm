/*
 * $Id: show.c 1.10 Broadcom SDK $
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
#include <appl/diag/ea/ea.h>

#include <bcm/stat.h>
#include <bcm/error.h>
#include <bcm/init.h>

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/drv_if.h>


#include <sal/appl/pci.h>

#if defined(VXWORKS)
#include <netShow.h>
#include <muxLib.h>
#endif /* VXWORKS */

/*
 * Clear something for a user
 */
cmd_result_t
cmd_ea_clear(int unit, args_t *a)
{
	char *parm = ARG_GET(a);
	pbmp_t pbmp;
	bcm_port_t port;

	if (!sh_check_attached(ARG_CMD(a), unit)){
		return CMD_FAIL;
	}

	if (!parm){
		return CMD_USAGE;
	}

	if (!sal_strcasecmp(parm, "counters") ||
		!sal_strcasecmp(parm, "c")){
		if ((parm = ARG_GET(a)) == NULL){
			pbmp = PBMP_ALL(unit);
		}else if (parse_pbmp(unit, parm, &pbmp) < 0){
			printk("%s: Invalid port bitmap: %s\n", ARG_CMD(a), parm);
			return CMD_FAIL;
		}
		for ((port) = 0; (port) < _SHR_PBMP_PORT_MAX; (port)++){
			if (_SHR_PBMP_MEMBER((pbmp), (port))){
				if (BCM_E_NONE != bcm_stat_clear(unit, port)){
					printk("ERROR: Clear port [%d] counter failed\n", port);
					continue;
				}
			}
		}
		return CMD_OK;
	}

	if (!sal_strcasecmp(parm, "stats")){
		int rv = CMD_OK;
		bcm_port_t port;

		if ((parm = ARG_GET(a)) == NULL){
			pbmp = PBMP_ALL(unit);
		}else if (parse_pbmp(unit, parm, &pbmp) < 0){
			printk("%s: Invalid port bitmap: %s\n", ARG_CMD(a), parm);
			return CMD_FAIL;
		}

		PBMP_ITER(pbmp, port){
			int rv;

			if ((rv = bcm_stat_clear(unit, port) != BCM_E_NONE)){
				printk("%s: Unit %d Port %d failed to clear stats: %s\n",
						ARG_CMD(a), unit, port, bcm_errmsg(rv));
				rv = CMD_FAIL;
			}
		}
		return rv;
	}

	return CMD_OK;
}

/*
 * Show something for user command line interface
 */
static parse_key_t show_arg[] = {
	"Pci",					/* 0 */
	"Counters",				/* 1 */
	"Errors",				/* 2 */
	"Interrupts",			/* 3 */
	"Chips",				/* 4 */
	"Statistics",			/* 5 */
	"MIB",					/* 6 */
};

static int show_arg_cnt	= PARSE_ENTRIES(show_arg);

static parse_key_t show_ctr_arg[] = {
	"Changed",				/* 0 */
	"Same",					/* 1 */
	"Zero",					/* 2 */
	"NonZero",				/* 3 */
	"Hex",					/* 4 */
	"Raw",					/* 5 */
	"All",					/* 6 */
	"ErDisc",				/* 7 */
};

static int show_ctr_arg_cnt = PARSE_ENTRIES(show_ctr_arg);

cmd_result_t
cmd_ea_show(int unit, args_t *a)
{
	soc_control_t *soc = SOC_CONTROL(unit);
	pbmp_t pbmp;
	const parse_key_t *cmd;
	char *c;
	int flags = 0;
	bcm_port_t port;

	if (!(c = ARG_GET(a))){	/* Nothing to do */
		return (CMD_USAGE); /* Print usage line */
	}
#if defined(VXWORKS)
#if defined(VXWORKS_NETWORK_STACK_6_5)
#if 0	
	if (!sal_strcasecmp(c, "ip")){
		netstat("-p ip -f inet");
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "icmp")){
		netstat("-p icmp -f inet");
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "arp")){
		arpShow();
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "udp")){
		netstat("-p udp -f inet");
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "tcp")){
		netstat("-p tcp -f inet");
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "mux")){
		muxShow(NULL, 0);
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "routes")){
		netstat("-a -r");
		return CMD_OK;
	}
#endif	
	if (!sal_strcasecmp(c, "hosts")){
		hostShow();
		return CMD_OK;
	}
#else /*!VXWORKS_NETWORK_STACK_6_5*/
	if (!sal_strcasecmp(c, "ip")){
		ipstatShow(FALSE);
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "icmp")){
		icmpstatShow();
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "arp")){
		arpShow();
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "udp")){
		udpstatShow();
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "tcp")){
		tcpstatShow();
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "mux")){
		muxShow(NULL, 0);
	}
	if (!sal_strcasecmp(c, "routes")){
		routeShow();
		return CMD_OK;
	}
	if (!sal_strcasecmp(c, "hosts")){
		hostShow();
		return CMD_OK;
	}
#endif
#endif

#if defined(BROADCOM_DEBUG)
	if (!sal_strcasecmp(c, "feature") || !sal_strcasecmp(c, "features")){
		soc_feature_t	f;

		c = ARG_GET(a);
		printk("Unit %d features:\n", unit);

		for (f = 0; f < soc_feature_count; f++){
			if (soc_feature(unit, f)){
				printk("\t%s\n", soc_feature_name[f]);
			}else{
				printk("\t[%s]\n", soc_feature_name[f]);
			}
		}
		return CMD_OK;
	}
#if defined(BCM_ROBO_SUPPORT)	
	if (!sal_strcasecmp(c, "param") || !sal_strcasecmp(c, "params")) {
        /* Current unit */	
        soc_robo_chip_dump(unit, SOC_DRIVER(unit));
		return CMD_OK;
	}
#endif
	if (!sal_strcasecmp(c, "unit") || !sal_strcasecmp(c, "units")){
		int u;
		soc_control_t *usoc;

		c = ARG_GET(a);
		if (c != NULL){
			u = sal_ctoi(c, 0);
			if (!SOC_UNIT_VALID(u)){
				printk("Unit %d is not valid\n", u);
				return CMD_FAIL;
			}
			usoc = SOC_CONTROL(u);
			if (!(usoc->soc_flags & SOC_F_ATTACHED)){
				printk("Unit %d (detached)\n", u);
				return CMD_OK;
			}
			printk("Unit %d chip %s%s\n",
					u,
					soc_dev_name(u),
					u == unit ? "current" : "");
		}else{
			for (u = 0; u < soc_ndev; u++){
				if (!SOC_UNIT_VALID(u)){
					continue;
				}
				usoc = SOC_CONTROL(u);
				printk("Unit %d chip %s%s\n",
						u,
						soc_dev_name(u),
						u == unit ? "(current)" : "");
			}
		}
		return CMD_OK;
	}
#endif
    cmd = parse_lookup(c, show_arg, sizeof(show_arg[0]), show_arg_cnt);
    if (!cmd) {
        printk("%s: Error: Invalid option %s\n", ARG_CMD(a), c);
        return(CMD_FAIL);
    }
    switch (cmd - show_arg){
    	case 0:
    		/*
    		 * PCI related functions not available yet.
    		 */
    		printk("The name is %s\n", soc_dev_name(unit));
    		break;
    	case 1:
    		/*
    		 * Counter related functions
    		 */
    	    cmd = parse_lookup(c, show_ctr_arg, sizeof(show_ctr_arg[0]), show_ctr_arg_cnt);
    	    switch(cmd - show_ctr_arg){
    	    	case 0:
    	    		flags |= SHOW_CTR_CHANGED;
    	    		break;
    	    	case 1:
    	    		flags |= SHOW_CTR_SAME;
    	    		break;
    	    	case 2:
    	    		flags |= SHOW_CTR_Z;
    	    		break;
    	    	case 3:
    	    		flags |= SHOW_CTR_NZ;
    	    		break;
    	    	case 4:
    	    		flags |= SHOW_CTR_HEX;
    	    		break;
    	    	case 5:
    	    		flags |= SHOW_CTR_RAW;
    	    		break;
    	    	case 6:
    	    		flags |= (SHOW_CTR_CHANGED | SHOW_CTR_SAME |
    	    				SHOW_CTR_Z | SHOW_CTR_NZ);
    	    		break;
    	    	case 7:
    	    		flags |= SHOW_CTR_ED;
    	    		break;
    	    	default:
    	    		return CMD_OK;

    	    }
    		break;
    	case 2:
    		/*
    		 * Errors related functions
    		 */
    		printk("%s: Errors: MII[%d]\n", ARG_CMD(a), soc->mmu_error_block);
    		break;
    	case 3:
    		/*
    		 * Interrupts related functions
    		 */
    		break;
    	case 4:
    		/*
    		 * Chips related functions
    		 */
    		printk("Know chips:\n");
    		soc_cm_display_known_devices();
    		break;
    	case 5:
    		/*
    		 * Statistics
    		 */
    	case 6:
    		/*
    		 * MIB
    		 */
    		flags = 0;
    		if ((c = ARG_GET(a)) != NULL){
    			if (0 != parse_pbmp(unit, c, &pbmp)){
    				printk("%s: Invalid ports:%s\n", ARG_CMD(a), c);
    				return (CMD_FAIL);
    			}
    			if ((c = ARG_GET(a)) != NULL && sal_strcasecmp(c, "all") == 0){
    				flags = 1;
    			}
    		}else{
    			pbmp = PBMP_ALL(unit);
    		}
			bcm_stat_sync(unit);
    		PBMP_ITER(pbmp, port){
    			bcm_stat_val_t s;
    			char *sname;
    			char *_stat_names[] = BCM_STAT_NAME_INITIALIZER;
    			int rv;
    			uint64 val;

    			if (!SOC_PORT_VALID(unit, port)){
    				break;
    			}
    			printk("%s: Statistics for unit %d port %s\n",
    					ARG_CMD(a), unit, SOC_PORT_NAME(unit, port));
    			printk("snmpValCount=%d\n", snmpValCount);

    			for (s = 0; s < snmpValCount; s++){
    				sname = _stat_names[s];
    				if (!sname){
    					continue;
    				}
    				rv = bcm_stat_get(unit, port, s, &val);
    				if (rv < 0){
    					printk("%8s\t%s (stat %d): %s\n",
    							"-", sname, s, bcm_errmsg(rv));
    					continue;
    				}
    				if (flags == 0 && COMPILER_64_IS_ZERO(val)){
    					continue;
    				}
    				if (COMPILER_64_IS_ZERO(val) == 0){
    					printk("%8u\t%s (stat %d)\n",
    							COMPILER_64_LO(val), sname, s);
    				}else {
    					printk("0x%08x%08x\t%s (stat %d)\n",
    							COMPILER_64_HI(val),
    							COMPILER_64_LO(val),
    							sname, s);
    				}
    			}
    		}
    		break;
    	default:
    		return CMD_FAIL;
    }
    return CMD_OK;
}
