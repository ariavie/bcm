
/*
 * $Id: ddr.c,v 1.2 Broadcom SDK $
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
 *
 */

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <bcm/error.h>
#include <sal/appl/sal.h>
#include <sal/appl/config.h>
#include <shared/bsl.h>
#ifdef BCM_DDR3_SUPPORT
#include <soc/shmoo_ddr40.h>
#include <soc/phy/ddr40.h>

char cmd_ddr_mem_write_usage[] = "\n"
" DDRMemWrite ci<n> range=0xstart-[0xend] data=0xdata\n"
" DDRMemWrite ci0,ci1 range=0x0\n"
" DDRMemWrite ci2 range=0x0-0x100"
"\n";

cmd_result_t
cmd_ddr_mem_write(int unit, args_t *a)
{
    uint32 data_wr[8] = {0};
    uint32 data = 0;
    int ci = 0;
    int addr = 0;
    int bank = 0;
    int row = 0;
    int col = 0;
    int rv_stat = CMD_OK;
    cmd_result_t ret_code;
    char *c = NULL;
    char *range = NULL;
    char *lo = NULL;
    char *hi = NULL;
    soc_pbmp_t ci_pbm;
    parse_table_t pt;
    int start_addr;
    int end_addr;
    int i;
    int rv;
    int inc;

    if (((c = ARG_GET(a)) == NULL) || (parse_pbmp(unit, c, &ci_pbm) < 0)) {
        return CMD_USAGE;
    }  

    if((lo = range = ARG_GET(a)) != NULL) {
        if((hi = sal_strchr(range, '-')) != NULL) {
            hi++;
        } else {
            hi = lo;
        }
    } else {
        return CMD_USAGE;
    }

    if (ARG_CNT(a)) {
        parse_table_init(0,&pt);
        parse_table_add(&pt,"data",PQ_INT,
                        0, &data, NULL);
        parse_table_add(&pt,"inc",PQ_INT,
                        0, &inc, NULL);
        if (!parseEndOk(a,&pt,&ret_code)) {
            return ret_code;
        }
    } else {
        return CMD_USAGE;
    }

    if (diag_parse_range(lo,hi,&start_addr,&end_addr,0,1<<22)) {
        cli_out("Invalid range. Valid range is : 0 - 0x%x\n",(1<<22));
        return CMD_FAIL;
    }

    /* for now 32B of data will be same */
    for(i=0;i<8;i++) {
        if (inc) {
            data_wr[i] = data+i;
        } else {
            data_wr[i] = data;
        }
    }

    SOC_PBMP_ITER(ci_pbm,ci) {
        cli_out("Writing ci%d DDR %s ..\n",ci,lo);
        for (addr=start_addr; addr <= end_addr; addr++) {
            bank = (addr & 0x7);
            col  = (addr >> 3) & 0x3f;
            row  = (addr >> 9) & 0x7fff;
            /* for debug */
            cli_out("Writing to ci%d,bank[%d],row[0x%04x],cols[0x%03x - 0x%03x] 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
                    ci,bank,row,col,col+0xf,data_wr[0],data_wr[1],data_wr[2],data_wr[3],data_wr[4],data_wr[5],data_wr[6],data_wr[7]);

            rv = soc_ddr40_write(unit, ci, addr,
                                 data_wr[0],data_wr[1],data_wr[2],
                                 data_wr[3],data_wr[4],data_wr[5],
                                 data_wr[6],data_wr[7]);
            if (rv != BCM_E_NONE) {
                rv_stat = CMD_FAIL;
            }
        }
    }

    return rv_stat;
}

char cmd_ddr_mem_read_usage[] = "\n"
" DDRMemRead ci<n> range=0xstart-[0xend]\n"
" DDRMemRead ci0,ci1 range=0x0\n"
" DDRMemRead ci2 range=0x0-0x100"
"\n";

cmd_result_t
cmd_ddr_mem_read(int unit, args_t *a)
{
    uint32 data_rd[8] = {0};
    int ci = 0;
    int addr = 0;
    int bank = 0;
    int row = 0;
    int col = 0;
    int rv_stat = CMD_OK;
    char *c = NULL;
    char *range = NULL;
    char *lo = NULL;
    char *hi = NULL;
    soc_pbmp_t ci_pbm;
    int start_addr;
    int end_addr;
    int rv;

    if (((c = ARG_GET(a)) == NULL) || (parse_pbmp(unit, c, &ci_pbm) < 0)) {
        return CMD_USAGE;
    }  

    if((lo = range = ARG_GET(a)) != NULL) {
        if((hi = sal_strchr(range, '-')) != NULL) {
            hi++;
        } else {
            hi = lo;
        }
    } else {
        return CMD_USAGE;
    }

    if (diag_parse_range(lo,hi,&start_addr,&end_addr,0,1<<22)) {
        cli_out("Invalid range. Valid range is : 0 - 0x%x\n",(1<<22));
        return CMD_FAIL;
    }

    SOC_PBMP_ITER(ci_pbm,ci) {
        cli_out("Reading ci%d DDR %s ..\n",ci,lo);
        for (addr=start_addr; addr <= end_addr; addr++) {
            rv = soc_ddr40_read(unit, ci, addr,
                                &data_rd[0],&data_rd[1],&data_rd[2],
                                &data_rd[3],&data_rd[4],&data_rd[5],
                                &data_rd[6],&data_rd[7]);

            if (rv == BCM_E_NONE) {
                /* convert the dram addr to pla_addr, to show bank,row,col */
                bank = (addr & 0x7);
                col  = (addr >> 3) & 0x3f;
                row  = (addr >> 9) & 0x7fff;
                cli_out("ci%d,bank[%d],row[%d],col[0x%03x - 0x%03x] = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
                        ci,bank,row,col,col+0xf,data_rd[0],data_rd[1],data_rd[2],data_rd[3],
                        data_rd[4],data_rd[5],data_rd[6],data_rd[7]);
            } else {
                rv_stat = CMD_FAIL;
            }
        }
    }
    return rv_stat;
}

char cmd_ddr_phy_read_usage[] = "\n"
" DDRPhyRead ci<n> [addr=<0xaddr>]\n"
" eg. DDRPhyRead ci0\n"
"     DDRPhyRead ci0,ci1 addr=0x0\n";


cmd_result_t
cmd_ddr_phy_read(int unit, args_t *a)
{
    uint32 data[4] = {0};
    int addr,address = -1;
    int reg = 0;
    int ci = 0;
    char *c = NULL;
    soc_pbmp_t ci_pbm;
    parse_table_t pt;
    cmd_result_t rv;

    if (((c = ARG_GET(a)) == NULL) || (parse_pbmp(unit, c, &ci_pbm) < 0)) {
        return CMD_USAGE;
    }

    if (ARG_CNT(a) > 0) {
        parse_table_init(0,&pt);
        parse_table_add(&pt,"addr",PQ_INT,
                        (void *) (0), &address, NULL);
        if (!parseEndOk(a,&pt,&rv)) {
            return rv;
        }
    }

    SOC_PBMP_ITER(ci_pbm,ci) {
        addr = address;
        if (addr == -1) {
            /* ADDR_CTL */
            cli_out("CI%d ( DDR40_PHY_ADDR_CTL )\n",ci);
            for (addr=DDR40_PHY_ADDR_CTL_MIN; addr <= DDR40_PHY_ADDR_CTL_MAX;) {
                for (reg=0;reg<4 && addr <= DDR40_PHY_ADDR_CTL_MAX;reg++) {
                    if(soc_ddr40_phy_reg_ci_read(unit, ci, addr, &data[reg])) {
                        cli_out("failed to read phy register 0x%04x\n", addr);
                    }
                    cli_out("    0x%04x: 0x%08x ",addr,data[reg]); 
                    addr += 4;
                }
                cli_out("\n");
            }

            /* BYTE_LANE0 */
            cli_out("CI%d ( DDR40_PHY_BYTE_LANE0 )\n",ci);
            for (addr=DDR40_PHY_BYTE_LANE0_ADDR_MIN; addr <= DDR40_PHY_BYTE_LANE0_ADDR_MAX;) {
                for (reg=0;reg<4 && addr <= DDR40_PHY_BYTE_LANE0_ADDR_MAX;reg++) {
                    if(soc_ddr40_phy_reg_ci_read(unit, ci, addr, &data[reg])) {
                        cli_out("failed to read phy register 0x%04x\n", addr);
                    }
                    cli_out("    0x%04x: 0x%08x ",addr,data[reg]); 
                    addr += 4;
                }
                cli_out("\n");
            }

            /* BYTE_LANE1 */
            cli_out("CI%d ( DDR40_PHY_BYTE_LANE1 )\n",ci);
            for (addr=DDR40_PHY_BYTE_LANE1_ADDR_MIN; addr <= DDR40_PHY_BYTE_LANE1_ADDR_MAX;) {
                for (reg=0;reg<4 && addr <= DDR40_PHY_BYTE_LANE1_ADDR_MAX;reg++) {
                    if(soc_ddr40_phy_reg_ci_read(unit, ci, addr, &data[reg])) {
                        cli_out("failed to read phy register 0x%04x\n", addr);
                    }
                    cli_out("    0x%04x: 0x%08x ",addr,data[reg]); 
                    addr += 4;
                }
                cli_out("\n");
            }
        } else {
            if(soc_ddr40_phy_reg_ci_read(unit, ci, addr, &data[0])) {
                cli_out("failed to read phy register 0x%04x\n", addr);
            }
            cli_out("    0x%04x: 0x%08x\n",addr,data[0]); 
        }
    }
    return CMD_OK;
}

char cmd_ddr_phy_write_usage[] = "\n"
" DDRPhyWrite ci<n> addr=<0xaddr> data=<0xdata>\n"
" eg. DDRPhyWrite ci addr=0x0 data=0x55\n"
"     DDRPhyWrite ci0,ci2 addr=0x240 data=0xaa\n";


cmd_result_t
cmd_ddr_phy_write(int unit, args_t *a)
{
    uint32 data = 0;
    int ci = 0;
    int addr = 0;
    parse_table_t pt;
    cmd_result_t rv;
    char *c = NULL;
    soc_pbmp_t ci_pbm;

    if (((c = ARG_GET(a)) == NULL) || (parse_pbmp(unit, c, &ci_pbm) < 0)) {
        return CMD_USAGE;
    }  

    if (ARG_CNT(a) == 2) {
        parse_table_init(0,&pt);
        parse_table_add(&pt,"addr",PQ_INT,
                        (void *) (0), &addr, NULL);
        parse_table_add(&pt,"data",PQ_INT,
                        (void *) (0), &data, NULL);
        if (!parseEndOk(a,&pt,&rv)) {
            return rv;
        }
    } else {
        cli_out("Invalid number of args.\n");
        return CMD_USAGE;
    }

    SOC_PBMP_ITER(ci_pbm,ci) {
        if(soc_ddr40_phy_reg_ci_write(unit, ci, addr, data)) {
            cli_out("Writing 0x%08x to ci:%d addr=0x%08x failed.\n",
                    data,ci,addr);
            return CMD_FAIL;
        }
    }
    return CMD_OK;
}

char cmd_ddr_phy_tune_usage[] = "\n"  
"  DDRPhyTune ci<n> \n"
" eg.  DDRPhyTune ci \n"
"      DDRPhyTune ci0,ci2\n";

cmd_result_t
cmd_ddr_phy_tune(int unit, args_t *a)
{
    parse_table_t pt;
    cmd_result_t rv;
    char *c = NULL;
    soc_pbmp_t ci_pbm;

    int ci      = 0;
    int phyType = 0;
    int ctlType = 1;
    int stat    = 0;
    int plot    = 0;
    int save    = 0;
    int restore = 0;

    if (((c = ARG_GET(a)) == NULL) || (parse_pbmp(unit, c, &ci_pbm) < 0)) {
        return CMD_USAGE;
    }  

    if (ARG_CNT(a) > 0) {
        parse_table_init(0,&pt);
        parse_table_add(&pt,"CtlType",PQ_INT,
                        (void *) (1), &ctlType, NULL);
        parse_table_add(&pt,"PhyType",PQ_INT,
                        (void *) (0), &phyType, NULL);
        parse_table_add(&pt,"Stat",PQ_INT,
                        (void *) (0), &stat, NULL);
        parse_table_add(&pt, "Plot", PQ_BOOL|PQ_DFL,
                        0, &plot, NULL);
        parse_table_add(&pt, "SaveCfg", PQ_BOOL|PQ_DFL,
                        0, &save, NULL);
        parse_table_add(&pt, "RestoreCfg", PQ_BOOL|PQ_DFL,
                        0, &restore, NULL);
        if (!parseEndOk(a,&pt,&rv)) {
            return rv;
        }
    }

    SOC_PBMP_ITER(ci_pbm,ci) {
        if (restore) {
            if (soc_ddr40_shmoo_restorecfg(unit, ci)) {
                cli_out(" RestoreCfg ci:%d failed\n", ci);
                return CMD_FAIL;
            }
        } else {
            if(soc_ddr40_shmoo_ctl(unit, ci, phyType, ctlType, stat , plot)) {
                cli_out(" ci:%d failed\n", ci);
                return CMD_FAIL;
            }
            if (save) {
                if (soc_ddr40_shmoo_savecfg(unit, ci)) {
                    cli_out(" SaveCfg ci:%d failed\n", ci);
                }
            }
        }
    }
    return CMD_OK;
}

#endif

