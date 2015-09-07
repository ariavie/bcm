/*
 * $Id: mcs.c,v 1.13 Broadcom SDK $
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
 * File: 	mcs.c
 * Purpose: 	Micro Controller Subsystem Support
 */

#include <assert.h>
#include <sal/core/libc.h>
#include <shared/bsl.h>
#include <bcm/error.h>
#include <soc/types.h>
#include <soc/dma.h>
#include <appl/diag/shell.h>
#include <appl/diag/system.h>

#if defined(BCM_CMICM_SUPPORT) || defined(BCM_IPROC_SUPPORT)

#include <soc/uc.h>
#include <soc/iproc.h>
#include <soc/shared/mos_coredump.h>
#include <soc/uc_msg.h>

extern void format_uint64_decimal(char *buf, uint64 n, int comma);

void dumpbuf(uint8 *buf, int count) {
    int i;
    uint8 *bp = buf;
    for(i = 0; i < count; i++) {
        cli_out("%02X", *bp++);
        if ((i % 16) == 15) {
            cli_out("\n");
        }
    }
}

#ifndef NO_FILEIO

#define DUMP_BUFFER_SIZE 1024

STATIC cmd_result_t
_mcs_dump_region(int unit, FILE *fp, uint32 *buffer, mos_coredump_region_t *reg) {

    uint32 addr = reg->baseaddr;
    uint32 len = reg->end - reg->start;
    int buf_idx = 0;
    while (len > 0) {
        buffer[buf_idx] = soc_uc_mem_read(unit, addr);

        len -= sizeof(uint32);
        addr += sizeof(uint32);

        /* Flush buffer to disk if it is full, or we are done with the region */
        if (++buf_idx >= (DUMP_BUFFER_SIZE / sizeof(uint32)) || (len == 0)) {
            if (sal_fwrite(buffer, 4, buf_idx, fp) != buf_idx) {
                return(CMD_FAIL);
            }
            buf_idx = 0;
        }
    } 

    return (CMD_OK);
}

STATIC cmd_result_t
mcs_dump(int unit, FILE *fp) {
    /* Set up the region */
    mos_coredump_region_t reg[6];
    uint32 *buffer;
    int i;
    int itcm_offset = 0x00000000;
    int itcm_size = 0x00020000;
    int dtcm_offset = 0x00020000;
    int dtcm_size = 0x00020000;
    int sram_size = 0x00100000;

    int core0_tcm_base = 0x00100000;
    int core1_tcm_base = 0x00200000;

    if (SOC_IS_KATANA(unit)) {
        /* defaults are correct */
#ifdef BCM_TRIUMPH3_SUPPORT
    } else if (SOC_IS_TRIUMPH3(unit)) {
        dtcm_offset = 0x00040000;
        dtcm_size = 0x00040000;
        sram_size = 0x00080000;
#endif
    } else {
        return CMD_FAIL;
    }

    /* Init the regions descriptors */
    reg[0].cores = 1;
    reg[0].baseaddr = core0_tcm_base + itcm_offset;
    reg[0].start = sizeof(reg);
    reg[0].end = reg[0].start + itcm_size;

    reg[1].cores = 1;
    reg[1].baseaddr = core0_tcm_base + dtcm_offset;
    reg[1].start = reg[0].end;
    reg[1].end = reg[1].start + dtcm_size;

    reg[2].cores = 2;
    reg[2].baseaddr = core1_tcm_base + itcm_offset;
    reg[2].start = reg[1].end;
    reg[2].end = reg[2].start + itcm_size;

    reg[3].cores = 2;
    reg[3].baseaddr = core1_tcm_base + dtcm_offset;
    reg[3].start = reg[2].end;
    reg[3].end = reg[3].start + dtcm_size;

    reg[4].cores = 3;
    reg[4].baseaddr = 0x400000;
    reg[4].start = reg[3].end;
    reg[4].end = reg[2].start + sram_size;

    reg[5].cores = 0;
    reg[5].baseaddr = 0;
    reg[5].start = 0;
    reg[5].end = 0;

    /* allocate 1k for data */
    buffer = (uint32 *) soc_cm_salloc(unit, DUMP_BUFFER_SIZE, "MCS Dump Buffer");
    if(buffer == NULL) {
        cli_out("Unable to allocate buffer\n");
        return(CMD_FAIL);
    }

    /* Write the desriptors */
    sal_memcpy(buffer, reg, sizeof(mos_coredump_region_t) * 6);
    /* Ensure that descriptors are in network byte order */
    for (i = 0; i < sizeof(mos_coredump_region_t) * 6 / sizeof(uint32); ++i) {
        buffer[i] = soc_htonl(buffer[i]);
    }

    if (sal_fwrite(buffer, sizeof(mos_coredump_region_t), 6, fp) != 6) {
        cli_out("Error writing header\n");
        return(CMD_FAIL);
    }

    for (i = 0; i < 6; i++) {
        if (_mcs_dump_region(unit, fp, buffer, &reg[i]) != CMD_OK) {
            cli_out("Error writing dump\n");
            return(CMD_FAIL);
        }
    } 
    
    soc_cm_sfree(unit, buffer);
    
    return(CMD_OK);
}

uint32 ihex_ext_addr[SOC_MAX_NUM_DEVICES]; /* Extended Address */


/*
 * returs -1 if error
 * returns number of bytes. (0 if this record doesn't contain any data).
 */
STATIC int
mcs_parse_ihex_record(int unit, char *line, uint32 *off)
{
    int count;
    uint32 address;

    switch (line[8]) {      /* Record Type */
        case '0' :      /* Data Record */
            count = (xdigit2i(line[1]) << 4) |
                    (xdigit2i(line[2]));

            address = (xdigit2i(line[3]) << 12) |
                      (xdigit2i(line[4]) << 8) |
                      (xdigit2i(line[5]) << 4) |
                      (xdigit2i(line[6]));

            *off = ihex_ext_addr[unit] + address;
            return count;
            break; /* Not Reachable */
        case '4' :      /* Extended Linear Address Record */
            address = (xdigit2i(line[9]) << 12) |
                      (xdigit2i(line[10]) << 8) |
                      (xdigit2i(line[11]) << 4) |
                      (xdigit2i(line[12]));

            ihex_ext_addr[unit] = (address << 16);
            cli_out("Exteded Linear Address 0x%x\n", ihex_ext_addr[unit]);
            return 0;
            break; /* Not Reachable */
        default :       /* We don't parse all other records */
            cli_out("Unsupported Record\n");
            return 0;
    }
    return -1; /* Why are we here? */ /* Not Reachable */
}

STATIC int
mcs_parse_srec_record (int unit, char *line, uint32 *off)
{
    int count;
    uint32 address;

    switch (line[1]) {      /* Record Type */
        case '0':           /* Header record - ignore */
            return 0;
            break; /* Not Reachable */
            
        case '1' :          /* Data Record with 2 byte address */
            count = (xdigit2i(line[2]) << 4) |
                    (xdigit2i(line[3]));
            count -= 3; /* 2 address + 1 checksum */

            address = (xdigit2i(line[4]) << 12) |
                      (xdigit2i(line[5]) << 8) |
                      (xdigit2i(line[6]) << 4) |
                      (xdigit2i(line[7]));

            *off = address;
            return count;
            break; /* Not Reachable */

        case '2' :          /* Data Record with 3 byte address */
            count = (xdigit2i(line[2]) << 4) |
                    (xdigit2i(line[3]));
            count -= 4; /* 3 address + 1 checksum */

            address = (xdigit2i(line[4]) << 20) |
                      (xdigit2i(line[5]) << 16) |
                      (xdigit2i(line[6]) << 12) |
                      (xdigit2i(line[7]) << 8) |
                      (xdigit2i(line[8]) << 4) |
                      (xdigit2i(line[9]));

            *off = address;
            return count;
            break; /* Not Reachable */

        case '3' :          /* Data Record with 4 byte address */
            count = (xdigit2i(line[2]) << 4) |
                    (xdigit2i(line[3]));
            count -= 5; /* 4 address + 1 checksum */

            address = (xdigit2i(line[4]) << 28) |
                      (xdigit2i(line[5]) << 24) |
                      (xdigit2i(line[6]) << 20) |
                      (xdigit2i(line[7]) << 16) |
                      (xdigit2i(line[8]) << 12) |
                      (xdigit2i(line[9]) << 8) |
                      (xdigit2i(line[10]) << 4) |
                      (xdigit2i(line[11]));

            *off = address;
            return count;
            break; /* Not Reachable */
            
        case '5':         /* Record count - ignore */
        case '6':         /* End of block - ignore */
        case '7':         /* End of block - ignore */
        case '8':         /* End of block - ignore */
        case '9':         /* End of block - ignore */
            return 0;
            break; /* Not Reachable */
            
        default :       /* We don't parse all other records */
            cli_out("Unsupported Record S%c\n", line[1]);
            return 0;
    }
    return -1; /* Why are we here? */ /* Not Reachable */
}


/*
 * It is assumed that by the time this routine is called
 * the record is already validated. So, blindly get data
 */
STATIC void
mcs_get_rec_data(char *line, int count, uint8 *dat)
{
    int i, datpos = 9;  /* 9 is valid for ihex */

    if (!count) {
        return;
    }

    if (line[0] != ':') {
        /* not ihex */
        switch (line[1]) {
        case '1':  /* S1 record */
            datpos = 8;
            break;
        case '2':  /* S2 record */
            datpos = 10;
            break;
        case '3':  /* S3 record */
            datpos = 12;
            break;
        default:
            cli_out("Unexpected record type: '%c'\n", line[1]);
            break;
        }
    }

    /*1 count = 2 hex chars */
    for(i = 0; i < count; i++) {
        *(dat + i) = ((xdigit2i(line[datpos + (i * 2)])) << 4) |
                    (xdigit2i(line[datpos + (i * 2) + 1]));
    }
}

STATIC cmd_result_t
mcs_file_load(int unit, FILE *fp, int ucnum, uint32 *lowaddr, int stage, uint32 twostageaddr)
{
    uint32 addr=0;
    int rv = 0;
    char input[256], *cp = NULL;
    unsigned char data[256];

    if (lowaddr != NULL) {
        *lowaddr = 0xffff0000;
    }
    ihex_ext_addr[unit] = 0; /* Until an extended addr record is met.. */

    while (NULL != (cp = sal_fgets(input, sizeof(input) - 1, fp))) {
        if (input[0] == 'S') {
            rv = mcs_parse_srec_record(unit, cp, &addr);
        } else if (input[0] == ':') {
            rv = mcs_parse_ihex_record(unit, cp, &addr);
        } else {
            cli_out("unknown Record Type\n");
            rv = -1;
        }

        if (-1 == rv) {
            return (CMD_FAIL);
        }

        if (rv % 4) {
            cli_out("record Not Multiple of 4\n");
            return (CMD_FAIL);
        }

        /* Do this only if first stage */        
        if (stage == 0) {
            /* track lowest seen address with data */
            if ((rv > 0) && (lowaddr != NULL) && (addr < *lowaddr)) {
                *lowaddr = addr;
            }
        }

        if ((stage == 0) && (addr >= twostageaddr)) {
		  /* No Need to parse teh remaining now... */
            return CMD_OK;
        }

        if ( ((stage == 0) && (addr < twostageaddr)) ||
             ((stage == 1) && (addr >= twostageaddr))) {
                mcs_get_rec_data(cp, rv, data);
                soc_uc_load(unit, ucnum, addr, rv, data);
        }
    }
    return(CMD_OK);
}

#endif


char mcsload_usage[] =
    "Parameters: <uCnum> <file.hex>\n\t\t"
#ifndef COMPILER_STRING_CONST_LIMIT    
    "[InitMCS|ResetUC|StartUC|StartMSG]=true|false]\n\t\t"
    "Load the MCS memory area from <file.srec>.\n\t"
    "uCnum = uC number to be loaded.\n\t"
    "InitMCS = Init the MCS (not just one UC) (false)\n\t"
    "ResetUC = Reset the uC (true)\n\t"
    "StartUC = uC out of halt after load (true)\n\t"
    "StartMSG = Start messaging (true)\n"
#endif    
    ;

cmd_result_t
mcsload_cmd(int unit, args_t *a)
/*
 * Function: 	mcsload_cmd
 * Purpose:	Load a file into MCS.
 * Parameters:	unit - unit
 *		a - args, each of the files to be displayed.
 * Returns:	CMD_OK/CMD_FAIL/CMD_USAGE
 */
{
    cmd_result_t rv = CMD_OK;
    parse_table_t  pt;
    char *c , *filename;
    int ucnum, resetuc = 1, startuc=1, startmsg=1, initmcs = 0, twostage = 0;
    uint32 twostageaddr = 0xffffffff;

#ifndef NO_FILEIO
#ifndef NO_CTRL_C
    jmp_buf ctrl_c;
#endif    
    FILE * volatile fp = NULL;
    uint32 lowaddr;
    char *version;
#endif

    if (!sh_check_attached("mcsload", unit)) {
        return(CMD_FAIL);
    }

    if (!soc_feature(unit, soc_feature_uc)) {
        return (CMD_FAIL);
    } 

    if (ARG_CNT(a) < 2) {
        return(CMD_USAGE);
    }

    c = ARG_GET(a);
    if (!isint(c)) {
        cli_out("%s: Error: uC Num not specified\n", ARG_CMD(a));
        return(CMD_USAGE);
    }

    ucnum = parse_integer(c);
    if (ucnum > 1) { 
        cli_out("Invalid uProcessor number: %d\n", ucnum); 
        return(CMD_FAIL);
    }

    c = ARG_GET(a);
    filename = c;
    if (filename == NULL) {
        cli_out("%s: Error: No file specified\n", ARG_CMD(a));
        return(CMD_USAGE);
    }

    if (ARG_CNT(a) > 0) {
        parse_table_init(unit, &pt);
        parse_table_add(&pt, "InitMCS", PQ_DFL|PQ_BOOL,
                        0, &initmcs,  NULL);
        parse_table_add(&pt, "ResetUC", PQ_DFL|PQ_BOOL,
                        0, &resetuc,  NULL);
        parse_table_add(&pt, "StartUC", PQ_DFL|PQ_BOOL,
                        0, &startuc,  NULL);
        parse_table_add(&pt, "StartMSG", PQ_DFL|PQ_BOOL,
                        0, &startmsg,  NULL);
        parse_table_add(&pt, "TwoStage", PQ_DFL|PQ_BOOL,
                        0, &twostage,  NULL);
        parse_table_add(&pt, "TwoStageAddress", PQ_DFL|PQ_INT,
                        0, &twostageaddr, NULL);
        if (!parseEndOk(a, &pt, &rv)) {
            if (CMD_OK != rv) {
                return rv;
            }
        }
    }

    if (twostage) {
        startuc = 1; /* Force Start */
    } else {
        twostageaddr = 0xffffffff;
    }
    
    /* check for simulation*/
    if (SAL_BOOT_BCMSIM) {
        return(rv);
    }

#ifdef NO_FILEIO
    cli_out("no filesystem\n");
#else
#ifndef NO_CTRL_C    
    if (!setjmp(ctrl_c)) {
        sh_push_ctrl_c(&ctrl_c);
#endif
	/* See if we can open the file before doing anything else */
        fp = sal_fopen(filename, "r");
        if (!fp) {
            cli_out("%s: Error: Unable to open file: %s\n",
                    ARG_CMD(a), filename);
            rv = CMD_FAIL;
#ifndef NO_CTRL_C    
            sh_pop_ctrl_c();
#endif
            return rv;
        }

        /* Reset the UC to stop messages before stopping the msg system */
        if (resetuc || initmcs) {
            soc_uc_reset(unit, ucnum);
        }

        if (initmcs) {
            soc_uc_init(unit);
        }
            
        soc_uc_preload(unit, ucnum);

        rv = mcs_file_load(unit, fp, ucnum, &lowaddr, 0, twostageaddr);

        if (startuc) {
            soc_uc_start(unit, ucnum, lowaddr);
            sal_usleep(100 * 1000);
        }

#if defined(BCM_IPROC_SUPPORT)
        if (twostage) {
            int retries;
            uint32 rval;
            rv = CMD_OK;
            if (soc_cm_get_bus_type(unit) & SOC_DEV_BUS_ALT) {
                WRITE_PAXB_1_PCIE_EP_AXI_CONFIGr(unit, 0xfffdff7f);
            } else {
                WRITE_PAXB_0_PCIE_EP_AXI_CONFIGr(unit, 0xfffdff7f);
            }

            cli_out("Waiting for Core-%d to start", ucnum);
            rval = 0;
            retries = 0;
            while (rval != 0x10c8ed) { /* Locked?? */
                sal_usleep(10 * 1000);
                rval = soc_cm_iproc_read(unit, 0x1b07f000);
                cli_out(".");
                if (++retries > 100) {
                    rv = CMD_FAIL;
                    break;
                }
            }
            /* L2$ Cache has been Initialized */

            if (rv == CMD_OK) {
                cli_out("\nWaiting for L2$ to be configured");
                rval = 0;
                retries = 0;
                while (rval != 0xdeadc0de) {
                    sal_usleep(10 * 1000);
                    rval = soc_cm_iproc_read(unit, twostageaddr);
                    cli_out(".");
                    if (++retries > 100) {
                        rv = CMD_FAIL;
                        break;
                    }
                }
                cli_out("\n");
            }
            /* L2$ Cache has been configured as RAM */

            /* Last read was for first line of 2nd stage.  So close/reopen
               so that next load doesn't miss that line
            */

            sal_fclose((FILE *)fp);
            fp = NULL;

            if (rv != CMD_OK) {
                /* Last Load failed: Timed Out */
#ifndef NO_CTRL_C
                sh_pop_ctrl_c();
#endif
                return rv;
            }

            fp = sal_fopen(filename, "r");
            if (!fp) {
                cli_out("%s: Error: Unable to open file: %s\n",
                        ARG_CMD(a), filename);
                rv = CMD_FAIL;
#ifndef NO_CTRL_C
                sh_pop_ctrl_c();
#endif
                return rv;
            }

            rv = mcs_file_load(unit, fp, ucnum, &lowaddr, 1, twostageaddr);
            soc_cm_iproc_write(unit, twostageaddr, 0x10aded); /*Loaded */
        }
#endif /* IPROC_SUPPORT */

        sal_fclose((FILE *)fp);
        fp = NULL;

        if (startmsg) {
            soc_cmic_uc_msg_start(unit);
            soc_cmic_uc_msg_uc_start(unit, ucnum);

            version = soc_uc_firmware_version(unit, ucnum);
            if (version) {
                cli_out("%s\n", version);
                soc_cm_sfree(unit, version);
            }
        }

#ifndef NO_CTRL_C    
    } else if (fp) {
        sal_fclose((FILE *)fp);
        fp = NULL;
        rv = CMD_INTR;
    }
    sh_pop_ctrl_c();
#endif    
#endif /* NO_FILEIO */
    sal_usleep(10000);

    return(rv);
}

char mcsdump_usage[] =
    "Parameters: <dumpfile> [Halt=true|false]\n\t"
    "Dump both uCs memory in BRCM dumpfile format.\n\t"
    "<file.hex> = dumpfile to write.\n\t"
    "Halt = Halt the UCs prior to the dump.\n";

cmd_result_t
mcsdump_cmd(int unit, args_t *a)
/*
 * Function: 	mcsdump_cmd
 * Purpose:	Dump the MCS.
 * Parameters:	unit - unit
 *		a - args, each of the files to be displayed.
 * Returns:	CMD_OK/CMD_FAIL/CMD_USAGE
 */
{
    cmd_result_t rv = CMD_OK;
    parse_table_t  pt;
    char *c , *filename;
    int halt = 0;
    uint32 reg0, reg1, regs;

#ifndef NO_FILEIO
#ifndef NO_CTRL_C
    jmp_buf ctrl_c;
#endif
    FILE * volatile fp = NULL;
#endif

    if (!sh_check_attached("mcsload", unit)) {
        return(CMD_FAIL);
    }

    /* CMICm only at the moment */
    if (!soc_feature(unit, soc_feature_mcs)) {
        return (CMD_FAIL);
    } 

    if (ARG_CNT(a) < 1) {
        return(CMD_USAGE);
    }

    c = ARG_GET(a);
    filename = c;
    if (filename == NULL) {
        cli_out("%s: Error: No file specified\n", ARG_CMD(a));
        return(CMD_USAGE);
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "Halt", PQ_DFL|PQ_BOOL,
                    0, &halt,  NULL);
    if (!parseEndOk(a, &pt, &rv)) {
        if (CMD_OK != rv) {
            return rv;
        }
    }


    if (halt) {
        /* Halt both uCs so that the dump is clean */
        READ_UC_0_RST_CONTROLr(unit, &reg0);
        regs = reg0;
        soc_reg_field_set(unit, UC_0_RST_CONTROLr, &regs, CPUHALT_Nf, 0);
        WRITE_UC_0_RST_CONTROLr(unit, regs);
            
        READ_UC_0_RST_CONTROLr(unit, &reg1);
        regs = reg0;
        soc_reg_field_set(unit, UC_0_RST_CONTROLr, &regs, CPUHALT_Nf, 0);
        WRITE_UC_1_RST_CONTROLr(unit, regs);
    }
        
#ifdef NO_FILEIO
    cli_out("no filesystem\n");
#else
#ifndef NO_CTRL_C 
    if (!setjmp(ctrl_c)) {
        sh_push_ctrl_c(&ctrl_c);
#endif

        fp = sal_fopen(filename, "w");
        if (!fp) {
            cli_out("%s: Error: Unable to open file: %s\n",
                    ARG_CMD(a), filename);
            rv = CMD_FAIL;
        } else {
            rv = mcs_dump(unit, fp);
            sal_fclose((FILE *)fp);
            fp = NULL;
        }
#ifndef NO_CTRL_C 
    } else if (fp) {
        sal_fclose((FILE *)fp);
        fp = NULL;
        rv = CMD_INTR;
    }

    sh_pop_ctrl_c();
#endif
#endif /* NO_FILEIO */

    if (halt) {
        /* Put both uCs back in previous state */
        WRITE_UC_0_RST_CONTROLr(unit, reg0);
        WRITE_UC_1_RST_CONTROLr(unit, reg1);
    }

        
    return(rv);
}

char mcsmsg_usage[] =
    "Parameters: [ucnum|Start|Stop]\n\t"
    "Control messaging with the UCs.\n\t"
    "ucnum = ucnum to start comminicating with (must start first).\n\t"
    "Init = Init messaging with all uCs.\n\t"
    "Halt = Halt messaging with all uCs.\n";

cmd_result_t
mcsmsg_cmd(int unit, args_t *a)
/*
 * Function: 	mcsmsg_cmd
 * Purpose:	Start messading with the MCS.
 * Parameters:	unit - unit
 *		a - args, 0, 1, stop.
 * Returns:	CMD_OK/CMD_FAIL/CMD_USAGE
 */
{
    char *c;
    int ucnum;

    if (!sh_check_attached("mcsmsg", unit)) {
        return(CMD_FAIL);
    }

    if (!soc_feature(unit, soc_feature_uc)) {
        return (CMD_FAIL);
    } 

    if (ARG_CNT(a) != 1) {
        return(CMD_USAGE);
    }

    c = ARG_GET(a);
    if (isint(c)) {
        ucnum = parse_integer(c);
        if (ucnum < CMICM_NUM_UCS) {
            soc_cmic_uc_msg_uc_start(unit, ucnum);
        } else {
            cli_out("%s: Error: uC Num not legal\n", ARG_CMD(a));
            return(CMD_USAGE);
        }
        
    } else if (sal_strcasecmp(c, "INIT") == 0) {
        soc_cmic_uc_msg_start(unit);
        
    } else if (sal_strcasecmp(c, "HALT") == 0) {
        soc_cmic_uc_msg_stop(unit);
        
    } else {
        cli_out("%s: Error: Invalid parameter\n", ARG_CMD(a));
        return(CMD_USAGE);
    } 

    return (CMD_OK);

}

void mcs_core_status(int unit, int uC, uint32 base)
{
    int i;
    uint32 cpsr;
    char *version;

    /* CPSR is at offset 0x60 from start of TCM */
    /* Core 0's TCM is at a PCI address of 0x100000, Core 1's is at 0x200000 */
    cpsr = soc_uc_mem_read(unit, base + 0x60);
    cli_out("uC %d status: %s\n", uC, (cpsr) ? "Fault" : "Ok");
    if (cpsr) {
        cli_out("\tcpsr\t0x%08x\n", cpsr);
        cli_out("\ttype\t0x%08x\n", soc_uc_mem_read(unit, base + 0x64));
        for (i = 0; i < 16; ++i) {
            cli_out("\tr%d\t0x%08x\n",
                    i, soc_uc_mem_read(unit, base + 0x20 + i*4));
        }
    }
    else {
        version = soc_uc_firmware_version(unit, uC);
        if (version) {
            cli_out("version: %s\n", version);
            soc_cm_sfree(unit, version);
        }
    }
}

char mcsstatus_usage[] =
    "Prints fault status of microcontrollers\n";
cmd_result_t
mcsstatus_cmd(int unit, args_t *a)
/*
 * Function: mcsstatus_cmd
 * Purpose: Show fault status of MCS
 * Parameters: unit - unit
 *      a - args (none)
 * Returns: CMD_OK/CMD_FAIL/CMD_USAGE
 */
{
    if (!sh_check_attached("mcsmsg", unit)) {
        return(CMD_FAIL);
    }

    if (!soc_feature(unit, soc_feature_uc)) {
        return (CMD_FAIL);
    }

    if (ARG_CNT(a) > 0) {
        uint32 addr = parse_integer(ARG_GET(a));
        cli_out("%08x: %08x\n", addr, soc_uc_mem_read(unit, addr));

        return(CMD_USAGE);
    }

    /* If a fault happens, the register dump will include a non-zero CPSR */

    if (soc_feature(unit, soc_feature_iproc)) {
        /* iProc - L2Cache + SRAM for now. */
        if (soc_uc_in_reset(unit, 0)) {
            cli_out("uC 0 status: RESET\n");
        }
        else {           
            mcs_core_status(unit, 0, 0x1b004000); /* 0x1b000000 for SRAM only */
        }
        if (soc_uc_in_reset(unit, 1)) {
            cli_out("uC 1 status: RESET\n");
        }
        else {           
            mcs_core_status(unit, 1, 0x1b034000);  /* 0x1b040000 for SRAM only */
        }
    }
    else {
        /* CMICm with TCMs */
        if (soc_uc_in_reset(unit, 0)) {
            cli_out("uC 0 status: RESET\n");
        }
        else {           
            mcs_core_status(unit, 0, 0x100000);
        }
        if (soc_uc_in_reset(unit, 1)) {
            cli_out("uC 1 status: RESET\n");
        }
        else {           
            mcs_core_status(unit, 1, 0x200000);
        }
    }

    return CMD_OK;
}


char mcstimestamp_usage[] =
    "Parameters: [event_id] [ENable|DISable]\n\t"
    "Display or configure timestamps captured by UC\n\t"
    "event_id = timestamp event index.  If empty, all will print.\n\t"
    "ENable = enable the specified timestamp event\n\t"
    "DISable = disable the specified timestamp event, unless used by firmware\n";

cmd_result_t
mcstimestamp_cmd(int unit, args_t *a)
{
    char uint64_decimal_string[27];
    int rv, i;
    int min_event_id = 0;
    int max_event_id = (soc_feature(unit, soc_feature_iproc)) ? 18 : 12;

    char *arg = ARG_GET(a);
    uint64 diff;

    if (!sh_check_attached("mcsmsg", unit)) {
        return(CMD_FAIL);
    }

    if (!soc_feature(unit, soc_feature_uc)) {
        return (CMD_FAIL);
    }

    if (arg) {
        int event_id;

        if (!isint(arg)) {
            return CMD_USAGE;
        }
        event_id = parse_integer(arg);

        arg = ARG_GET(a);
        if (arg) {
            if (parse_cmp("ENable", arg, 0)) {
                if (SOC_FAILURE(soc_cmic_uc_msg_timestamp_enable(unit, event_id))) {
                    return CMD_FAIL;
                } else {
                    return CMD_OK;
                }
            }
            if (parse_cmp("DISable", arg, 0)) {
                if (SOC_FAILURE(soc_cmic_uc_msg_timestamp_disable(unit, event_id))) {
                    return CMD_FAIL;
                } else {
                    return CMD_OK;
                }
            }

            return CMD_USAGE;
        }

        /* only a single integer argument */
        min_event_id = event_id;
        max_event_id = event_id;
    }

    cli_out(" #  Last Timestamp    Prev Timestamp    <difference>  Equivalent TS1     Equivalent Full Time\n");

    for (i = min_event_id; i <= max_event_id; ++i) {
        soc_cmic_uc_ts_data_t ts_data;

        rv = soc_cmic_uc_msg_timestamp_get(unit, i, &ts_data);
        if (SOC_FAILURE(rv)) {
            cli_out("soc_cmic_uc_msg_timestamp_get failed: %s (%d)\n", soc_errmsg(rv), rv);
        } else {
            diff = ts_data.hwts;
            COMPILER_64_SUB_64(diff, ts_data.prev_hwts);

            format_uint64_decimal(uint64_decimal_string, ts_data.time.seconds, 0);

            cli_out("%2d: %08x:%08x %08x:%08x <%10u>  %08x:%08x  %s.%09u\n",
                    i,
                    (unsigned)COMPILER_64_HI(ts_data.hwts), (unsigned)COMPILER_64_LO(ts_data.hwts),
                    (unsigned)COMPILER_64_HI(ts_data.prev_hwts), (unsigned)COMPILER_64_LO(ts_data.prev_hwts),
                    (unsigned)COMPILER_64_LO(diff),
                    (unsigned)COMPILER_64_HI(ts_data.hwts_ts1), (unsigned)COMPILER_64_LO(ts_data.hwts_ts1),
                    uint64_decimal_string, (unsigned)ts_data.time.nanoseconds);
        }
    }

    return CMD_OK;
}


#endif /* CMICM support */
