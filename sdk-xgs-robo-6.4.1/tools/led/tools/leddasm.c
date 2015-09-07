// $Id: leddasm.c,v 1.2 Broadcom SDK $
// $Copyright: Copyright 2012 Broadcom Corporation.
// This program is the proprietary software of Broadcom Corporation
// and/or its licensors, and may only be used, duplicated, modified
// or distributed pursuant to the terms and conditions of a separate,
// written license agreement executed between you and Broadcom
// (an "Authorized License").  Except as set forth in an Authorized
// License, Broadcom grants no license (express or implied), right
// to use, or waiver of any kind with respect to the Software, and
// Broadcom expressly reserves all rights in and to the Software
// and all intellectual property rights therein.  IF YOU HAVE
// NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
// IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
// ALL USE OF THE SOFTWARE.  
//  
// Except as expressly set forth in the Authorized License,
//  
// 1.     This program, including its structure, sequence and organization,
// constitutes the valuable trade secrets of Broadcom, and you shall use
// all reasonable efforts to protect the confidentiality thereof,
// and to use this information only in connection with your use of
// Broadcom integrated circuit products.
//  
// 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
// PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
// REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
// OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
// DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
// NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
// ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
// CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
// OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
// 
// 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
// BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
// INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
// ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
// TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
// THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
// WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
// ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
//
// This is a simple disassembler for the embedded processor used in the
// SM-Lite LED controller.  The particulars of the instruction set
// are documented elsewhere.
//
// The code is structured such that a portion of the code can be
// extracted as a stand-alone disassembler.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// prototype for external routine
int disassem(int b1, int b2, char *buf);


// --------------------------- driver program --------------------------

typedef unsigned char uint8;
uint8 g_program[256];	// holds program being inspected


// this fills the program ram from a file in a simple format:
// there are no addresses, just 16 lines of 16 bytes per line,
// printed in upper case hex.
void
read_hex_file(FILE *fp)
{
    int i;

    for(i=0; i<256; i++) {
	int byte;
	if (fscanf(fp,"%02X", &byte) == 0) {
	    printf("Error reading hex file: failure on the %d-th byte\n", i);
	    exit(-1);
	}
	g_program[i] = byte;
    }
}


void
dasm_file(char *basename)
{
    char name[256];
    char line[256];
    FILE *fp = 0;
    int pc, last_byte, instr_len;

    strcpy(name,basename);
    strcat(name,".hex");

    fp = fopen(name, "r");
    if (!fp) {
	printf("Error opening source file '%s'\n", name);
	exit(-1);
    }

    // read input file into g_program[] array
    read_hex_file(fp);
    fclose(fp);

    // figure out length of program -- assume end is padded with 00s
    last_byte = 0;
    for(last_byte=255; last_byte > 0; last_byte--) {
	if (g_program[last_byte] != 0x00)
	    break;
    }

    for(pc=0; pc<=last_byte; pc+=instr_len) {
	uint8 b1 = g_program[pc];
	uint8 b2 = g_program[pc+1];
	instr_len = disassem(b1,b2,line);
	if (instr_len == 0)
	    instr_len = 1;	// illegal op -- make forward progress anyway
	if (instr_len == 2)
	    printf("%02X: %02X %02X  %s\n", pc, g_program[pc],
						g_program[pc+1], line);
	else
	    printf("%02X: %02X     %s\n", pc, g_program[pc], line);
    }
}


void
help(void)
{
    printf("Usage: leddasm <filename>\n");
    printf("   This scans <filename>.hex, and prints out a disassembled listing\n");
    exit(-1);
}


int
main(int argc, char *argv[])
{
    if (argc != 2)
	help();

    if (strlen(argv[1]) > 250) {
	printf("How about a shorter source file name?\n");
	exit(-1);
    }

    dasm_file(argv[1]);

    return 0;
}
