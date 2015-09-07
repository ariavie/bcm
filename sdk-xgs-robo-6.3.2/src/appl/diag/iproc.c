/*
 * $Id: iproc.c 1.3 Broadcom SDK $
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
 * File: iproc.c
 * Purpose: 	IPROC memory Access Support
 */

#include <assert.h>
#include <sal/core/libc.h>
#include <bcm/error.h>
#include <soc/types.h>
#include <soc/dma.h>
#include <appl/diag/shell.h>
#include <appl/diag/system.h>

#ifdef BCM_IPROC_SUPPORT

#define DUMP_BUFFER_LEN 1024
#define DUMP_BUFFER_SIZE (DUMP_BUFFER_LEN * 4)

#define SWAP_ENDIAN(x) ( \
    (((x) << 24) & 0xff000000) | \
    (((x) <<  8) & 0x00ff0000) | \
    (((x) >>  8) & 0x0000ff00) | \
    (((x) >> 24) & 0x000000ff))

STATIC cmd_result_t
_iproc_write_from_args(int unit, int ce, uint32 addr, args_t *args) {
    char *c;
    uint32 data;
    int len, i;

    len = ARG_CNT(args);

    for (i = 0; i < len; i++) {
        c = ARG_GET(args);
        if (!isint(c)) {
            printk("%s: Error: Non Numeric Data %s\n", ARG_CMD(args), c);
            return CMD_FAIL;
        }
        data = parse_address(c);
        if (ce) {
            data = SWAP_ENDIAN(data);
        }
        soc_cm_iproc_write(unit, (addr + (i * 4)), data);
    }

    soc_cm_debug(DK_VERBOSE, "Wrote %d words to iProc 0x%08x\n", len, addr);
    return CMD_OK;
}

#ifndef NO_FILEIO

STATIC cmd_result_t
_iproc_write_from_bin_file(int unit, int ce, uint32 addr, FILE *fp) {
    uint32 data;
    int len, i;

    len = 0;
    while ((i = sal_fread(&data,4,1,fp)) != 0) {
        if (ce) {
            data = SWAP_ENDIAN(data);
        }
        soc_cm_iproc_write(unit, (addr + (len * 4)), data);
        len += i;
    }

    soc_cm_debug(DK_VERBOSE, "Wrote %d words to iProc 0x%08x\n", len, addr);
    return CMD_OK;
}

#endif /* NO_FILEIO */

STATIC cmd_result_t
_iproc_dump(int unit, int ce, void *fp, uint32 addr, int len) {
    uint32 rval;
    int i;
#ifndef NO_FILEIO
    FILE * fileptr = fp;
#endif
    printk("Addr 0x%x, len %d\n", addr, len);

    for (i = 0; i < len; i++) {
        rval = soc_cm_iproc_read(unit, (addr + (i * 4)));
        if (ce) {
            rval = SWAP_ENDIAN(rval);
        }
        printk("0x%08x ", rval);
        if ((i % 4) == 3) {
            printk ("\n");
        }
#ifndef NO_FILEIO
        if (fileptr != NULL) {
            fwrite(&rval, 4, 1, fileptr);
        }
#endif
    }
    printk ("\n");

    return CMD_OK;
}

char iprocwrite_usage[] =
    "Parameters: [ChangeEndian=0|1] <address> {<data>|<file.hex>}\n\t"
    "Writes to the IPROC Memory space.\n\t"
    "Either provide word data separated by space, or filename.\n\t"
    "Example: iprocwrite 0x48000000 0x123 0x45678 (Write 2 words)\n\t"
    "         iprocwrite 0x48000000 file.bin\n";

cmd_result_t
iprocwrite_cmd(int unit, args_t *args)
{
    cmd_result_t rv = CMD_OK;
    parse_table_t  pt;
    char *c, *filename = NULL;
    int ce = 0;
    uint32 addr = 0;
#ifndef NO_FILEIO
  #ifndef NO_CTRL_C
    jmp_buf ctrl_c;
  #endif
    FILE * volatile fp = NULL;
#endif

    if (ARG_CNT(args) < 1) {
        return(CMD_USAGE);
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "ChangeEndian", PQ_DFL|PQ_BOOL,
                    0, &ce,  NULL);
    if (parse_arg_eq(args, &pt) < 0) {
        printk("%s: Error: Unknown option: %s\n", ARG_CMD(args), ARG_CUR(args));
        parse_arg_eq_done(&pt);
        return(CMD_USAGE);
    }

    c = ARG_GET(args);
    if (!isint(c)) {
        printk("%s: Error: Address not specified\n", ARG_CMD(args));
        return(CMD_USAGE);
    }
    addr = parse_address(c);

    if (ARG_CNT(args) <= 0) {
        printk("%s: Error: Data / Filename not specified\n", ARG_CMD(args));
        return(CMD_USAGE);
    }

    c = ARG_GET(args);

    if (!isint(c)) {
        filename = c;
    }

    if (filename == NULL) {
        ARG_PREV(args);
        rv = _iproc_write_from_args(unit, ce, addr, args);
    } else {
#ifdef NO_FILEIO
        printk("no filesystem\n");
#else
        /* Read from file */        
  #ifndef NO_CTRL_C
          if (!setjmp(ctrl_c)) {
              sh_push_ctrl_c(&ctrl_c);
  #endif
    
              fp = sal_fopen(filename, "r");
              if (!fp) {
                  printk("%s: Error: Unable to open file: %s\n",
                     ARG_CMD(args), filename);
                  rv = CMD_FAIL;
  #ifndef NO_CTRL_C
                  sh_pop_ctrl_c();
  #endif
                  return(rv);
              } else {
                  rv = _iproc_write_from_bin_file(unit, ce, addr, fp);
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
#endif
    }

    return(rv);
}

char iprocread_usage[] =
    "Parameters: [ChangeEndian=0|1] <address> [<length>] [<file.bin>]\n\t"
    "Prints <length> words from the IPROC Memory space.\n\t"
    "And optionally saves to a file in binary format.\n\t"
    "  <length> defaults to 1 if not specified.\n\t"
    "  <file.bin> = dumpfile to write.\n";

cmd_result_t
iprocread_cmd(int unit, args_t *args)
{
    cmd_result_t rv = CMD_OK;
    parse_table_t  pt;
    char *c, *filename = NULL;
    uint32 addr = 0;
    int ce = 0;
    int len=1;
#ifndef NO_FILEIO
#ifndef NO_CTRL_C
    jmp_buf ctrl_c;
#endif
    FILE * volatile fp = NULL;
#endif

    if (ARG_CNT(args) < 1) {
        return(CMD_USAGE);
    }

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "ChangeEndian", PQ_DFL|PQ_BOOL,
                    0, &ce,  NULL);
    if (parse_arg_eq(args, &pt) < 0) {
        printk("%s: Error: Unknown option: %s\n", ARG_CMD(args), ARG_CUR(args));
        parse_arg_eq_done(&pt);
        return(CMD_USAGE);
    }

    c = ARG_GET(args);
    if (!isint(c)) {
        printk("%s: Error: Address not specified\n", ARG_CMD(args));
        return(CMD_USAGE);
    }
    addr = parse_address(c);

    if (ARG_CNT(args) > 0) {
        c = ARG_GET(args);
        if (isint(c)) {
            len = parse_address(c);
            if (ARG_CNT(args) > 0) {
                filename = ARG_GET(args);
            }
        } else {
            filename = c;
        }
    }

    if (filename == NULL) {
        /* Just dump to screen */
        rv = _iproc_dump(unit, ce, NULL, addr, len);
    } else {
#ifdef NO_FILEIO
        printk("no filesystem\n");
#else
        /* Dump to file */        
  #ifndef NO_CTRL_C
        if (!setjmp(ctrl_c)) {
            sh_push_ctrl_c(&ctrl_c);
  #endif

            fp = sal_fopen(filename, "w");
            if (!fp) {
                printk("%s: Error: Unable to open file: %s\n",
                   ARG_CMD(args), filename);
                rv = CMD_FAIL;
  #ifndef NO_CTRL_C
                sh_pop_ctrl_c();
  #endif
                return(rv);
            } else {
                soc_cm_debug(DK_VERBOSE, "Dump to file %s\n", filename);
                rv = _iproc_dump(unit, ce, fp, addr, len);
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
        sal_usleep(10000);
#endif /* NO_FILEIO */
    }

    return(rv);
}

#endif /* IPROC support */

