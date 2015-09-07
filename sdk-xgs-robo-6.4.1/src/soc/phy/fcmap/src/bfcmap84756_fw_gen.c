/*
 * $Id: bfcmap84756_fw_gen.c,v 1.7 Broadcom SDK $
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
 * Following method is used to download firmware into 8051 via the MDIO
 *
 * 1> Compile the Program in Keil compiler
 *    Select the hex file generation in Keil Compiler.
 *    Download hex2bin utility.
 *
 * 2> Use hex2bin to convert hex to bin file as
 *    hex2bin -v test.hex  (-v retains all the bytes with hex2bin strips
 *    otherwise)
 *
 * 3> Use xxd utility to convert to c include file as 
 *    xxd -i test.bin > test.h
 *
 *    Use following c program to generate a soc script to download the
 *    firmware file
 *
 *    The following c file assumes the 8051 executable in a byte array names
 *    test[];
 *    with fcmap_fw_data_len = length of the array.
 *    gcc -I <current dir> firmware.c
 *    ./a.out > write_mem.soc
 *
 *    The write_mem.soc file will download the 8051 executable via MDIO
 *
 *
 * Tool to do above steps:
 * ../tools/internal/chex uc_fc_firmware.hex bfcmap84756_fw_gen.c bfcmap84756_a0
 *
 * Above steps in a script file:
 *       hex2bin  -v  $1
 *       cp `basename $1 .hex`.bin test
 *       
 *       xxd -i test | sed 's/^\(.*test\)/STATIC \1/'  > test.h
 *       gcc -I.  $2
 *       ./a.out  | sed 's/bfcmap84756/'$3'/' > $3_fw.c
 *       
 *
 */
#define STATIC static
#include "test.h"
#include <stdio.h>
int main() {
    int i;
    unsigned char chksum0 = 0;
    unsigned char chksum1 = 0;
    unsigned char chksum2 = 0;
    unsigned char chksum3 = 0;
    
    for(i=0; i < fcmap_fw_data_len-4; i+=4) {
        chksum0 ^= fcmap_fw_data[i];
        chksum1 ^= fcmap_fw_data[i+1];
        chksum2 ^= fcmap_fw_data[i+2];
        chksum3 ^= fcmap_fw_data[i+3];
    }
    /* Handle the remainder */
    if (i < fcmap_fw_data_len) {
        chksum0 ^= fcmap_fw_data[i];
    }
    if ((i+1) < fcmap_fw_data_len) {
        chksum1 ^= fcmap_fw_data[i+1];
    }    
    if ((i+2) < fcmap_fw_data_len) {
        chksum2 ^= fcmap_fw_data[i+2];
    }    
    if ((i+3) < fcmap_fw_data_len) {
        chksum3 ^= fcmap_fw_data[i+3];
    }    
    printf("/*\n");
    printf(" * %c%s%c\n",'$',"Id",'$');
    printf(" *                                            \n");
    printf(
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
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$                      \n");
    printf(" *                                            \n");
    printf(" * DO NOT EDIT THIS FILE!                     \n");
    printf(" * This file is auto-generated.               \n");
    printf(" * Edits to this file will be lost when it is regenerated.\n");
    printf(" */\n");

    printf("\n\
#include <bfcmap.h>\n\
#include <bfcmap_int.h>\n\
#include <mmi_cmn.h>\n\
#include <stdio.h>\n\
");

    printf("\n");
    printf("#if defined(BCM84756_A0)\n");
    printf("\n");

    fflush(stdout);
    system("cat test.h");

    printf("\n");
    printf("\n");
    printf("STATIC char firmware_version[256] = \"%c%s%c\";\n",'$',"Id",'$');

    printf("\n\
\n\
STATIC int cl45_dev = 30 ;\n\
/*********************************************************\n\
 * Local functions and structures\n\
 *********************************************************/\n\
STATIC int\n\
_bfcmap_mdio_read(bfcmap_dev_addr_t dev_addr, \n\
                        buint32_t io_addr, buint16_t *data)\n\
{\n\
    io_addr = BLMI_IO_CL45_ADDRESS(cl45_dev, io_addr);\n\
    return _blmi_mmi_rd_f(dev_addr, io_addr, data);\n\
}\n\
\n\
STATIC int\n\
_bfcmap_mdio_write(bfcmap_dev_addr_t dev_addr, \n\
                        buint32_t io_addr, buint16_t data)\n\
{\n\
    io_addr = BLMI_IO_CL45_ADDRESS(cl45_dev, io_addr);\n\
    return _blmi_mmi_wr_f(dev_addr, io_addr, data);\n\
}\n\
\n\
#define BFCMAP_FW_READ(dev_addr, r, pd)   \\\n\
                    _bfcmap_mdio_read((dev_addr), (r), (pd))\n\
\n\
#define BFCMAP_FW_WRITE(dev_addr, r, d)   \\\n\
                    _bfcmap_mdio_write((dev_addr), (r), (buint16_t) (d))\n\
\n\
\n\
char * bfcmap84756_firmware_version(void)\n\
{\n\
    return firmware_version;\n\
}\n\
\n\
");

printf("\n\
int bfcmap84756_firmware_download(bfcmap_dev_addr_t dev_addr, int broadcast, int enable)\n\
{\n\
    int i;\n\
    unsigned short val;\n\
    buint16_t   rval, rval0, rval1;\n\
    int         rv = BFCMAP_E_NONE;\n\
    unsigned char chksum0 = 0x%x;\n\
    unsigned char chksum1 = 0x%x;\n\
    unsigned char chksum2 = 0x%x;\n\
    unsigned char chksum3 = 0x%x;\n\
    \n\
    \n\
    ", chksum0, chksum1, chksum2, chksum3);

    printf("#if 0 /* FOR REFERENCE */  \n");
    printf("phy raw C45 0x40 0x1E 0x20 0x%%04x = fcmap_fw_data_len\n");
    printf("phy raw C45 0x40 0x1E 0x20\n");
    printf("phy raw C45 0x40 0x1E 0x21 0x0000\n");
    printf("phy raw C45 0x40 0x1E 0x21\n");
    printf("phy raw C45 0x40 0x1E 0x2a 0x1000\n");
    printf("phy raw C45 0x40 0x1E 0x2a\n");
    printf("phy raw C45 0x40 0x1E 0x22 0x0009\n");
    printf("phy raw C45 0x40 0x1E 0x22\n");
    printf("; ;;;;;;;;;;;;;;\n");
    printf("; ;;;;;;;;;;;;;;\n");

    /* First steps */
    printf("\n\
    for(i=0; i < fcmap_fw_data_len; i+=2) {\n\
         val = (fcmap_fw_data[i+1] << 8) | fcmap_fw_data[i];\n\
         printf(\"phy raw C45 0x40 0x1E 0x23 0x%%x\\n\",val);\n\
    }\n\
    ");

    printf("; ;;;;;;;;;;;;;;\n");
    printf("phy raw C45 0x40 0x1E 0x22 0x0002\n");
    printf("phy raw C45 0x40 0x1E 0x22\n");
    printf("phy raw C45 0x40 0x1E 0x28 0x083c\n");
    printf("phy raw C45 0x40 0x1E 0x28\n");
    printf("phy raw C45 0x40 0x1E 0x22 0x0010\n");
    printf("phy raw C45 0x40 0x1E 0x22\n");
    printf("phy raw C45 0x40 0x1E 0x29\n");
    printf("sleep 1\n");
    printf("phy raw C45 0x40 0x1E 0x29\n");
    printf("sleep 1\n");
    printf("phy raw C45 0x40 0x1E 0x29\n");
    printf("sleep 1\n");
    printf("phy raw C45 0x40 0x1E 0x29\n");
    printf("sleep 1\n");
    printf("phy raw C45 0x40 0x1E 0x29\n");
    printf("sleep 1\n");
    printf("phy raw C45 0x40 0x1E 0x29\n");
    printf("#endif \n");
    printf("\n");
    printf("\n");

    /* check if the FC uCode is already downloaded */
    printf("\tBFCMAP_FW_WRITE(dev_addr, 0x20, 4);\n");
    printf("\tBFCMAP_FW_READ(dev_addr, 0x20, &rval );\n");
    printf("\tBFCMAP_FW_WRITE(dev_addr, 0x21,  0xDFFC);\n");
    printf("\tBFCMAP_FW_READ(dev_addr, 0x21, &rval );\n");
    printf("\tBFCMAP_FW_WRITE(dev_addr, 0x2a,  0x1000);\n");
    printf("\tBFCMAP_FW_READ(dev_addr, 0x2a, &rval );\n");
    printf("\tBFCMAP_FW_WRITE(dev_addr, 0x22,  0x0005);\n");
    printf("\tBFCMAP_FW_READ(dev_addr, 0x22, &rval );\n");
    printf("\tBFCMAP_FW_READ(dev_addr, 0x24, &rval0 );\n");
    printf("\tBFCMAP_FW_READ(dev_addr, 0x24, &rval1 );\n");
    printf("\tBFCMAP_FW_WRITE(dev_addr, 0x22,  0x0002);\n");
    
    printf("\tif ((rval0 != (chksum0 << 8 | chksum1)) || (rval1 != (chksum2 << 8 | chksum3))) {\n");
        printf("\t\tif (broadcast)\n");
            printf("\t\t\tprintf(\"Broadcast download FC for dev_addr = 0x%%x\\n\", (int)dev_addr);\n");
        printf("\t\telse\n");
            printf("\t\t\tprintf(\"Download FC for dev_addr = 0x%%x\\n\", (int)dev_addr);\n");
            
        printf("\t\tif (fcmap_fw_data_len % 2)\n");
              printf("\t\t\tBFCMAP_FW_WRITE(dev_addr, 0x20,  fcmap_fw_data_len+1);\n");
        printf("\t\telse\n");
              printf("\t\t\tBFCMAP_FW_WRITE(dev_addr, 0x20,  fcmap_fw_data_len);\n");
        printf("\t\tBFCMAP_FW_READ(dev_addr, 0x20, &rval );\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x21,  0x0000);\n");
        printf("\t\tBFCMAP_FW_READ(dev_addr, 0x21, &rval );\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x2a,  0x1000);\n");
        printf("\t\tBFCMAP_FW_READ(dev_addr, 0x2a, &rval );\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x22,  0x0009);\n");
        printf("\t\tBFCMAP_FW_READ(dev_addr, 0x22, &rval );\n");
        /* First steps */
        printf("\t\tfor(i=0; i < fcmap_fw_data_len-2; i+=2) {\n");
                 printf("\t\t\tval = (fcmap_fw_data[i+1] << 8) | fcmap_fw_data[i];\n");
            printf("\t\t\tBFCMAP_FW_WRITE(dev_addr, 0x23,  val);\n");
        printf("\t\t}\n");
         
        printf("\t\tif (i == fcmap_fw_data_len - 1)\n");
            printf("\t\t\tval = fcmap_fw_data[i];\n");
        printf("\t\telse\n");
             printf("\t\t\tval = (fcmap_fw_data[i+1] << 8) | fcmap_fw_data[i];\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x23,  val);\n");

        /* wait to make sure mdio writes are completed */
        printf("\t\tBFCMAP_SAL_USLEEP(50000);\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x22,  0x0002);\n");

        /* write the checksum to the end of the program memory */
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x20, 4);\n");
        printf("\t\tBFCMAP_FW_READ(dev_addr, 0x20, &rval );\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x21,  0xDFFC);\n");
        printf("\t\tBFCMAP_FW_READ(dev_addr, 0x21, &rval );\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x2a,  0x1000);\n");
        printf("\t\tBFCMAP_FW_READ(dev_addr, 0x2a, &rval );\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x22,  0x0009);\n");
        printf("\t\tBFCMAP_FW_READ(dev_addr, 0x22, &rval );\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x23, chksum0 << 8 | chksum1);\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x23, chksum2 << 8 | chksum3 );\n");
        /* wait to make sure mdio writes are completed */
        printf("\t\tBFCMAP_SAL_USLEEP(50000);\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x22,  0x0002);\n");
    printf("\t}\n");

   
    /* restart uC if FC is enabled */
    printf("\tif (enable) {\n");
        printf("\t\tBFCMAP_FW_WRITE(dev_addr, 0x22,  0x0010);\n");
        printf("\t\tBFCMAP_SAL_USLEEP(50000);\n");
    printf("\t}\n");
    
    printf("\treturn rv;\n");


    printf("}\n");
    printf("#endif /*BCM84756_A0*/\n");
}

