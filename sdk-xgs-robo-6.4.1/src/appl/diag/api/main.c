/* 
 * $Id: f2b74de6dd71b89663ef260bb087c0f4826a5676 $
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
 * File:        main.c
 * Purpose:     API mode test driver
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "api_mode_yy.h"
#include "api_grammar.tab.h"

#if defined(APIMODE_INCLUDE_MAIN)

static void
usage(void)
{
    fprintf(stderr, "apimode [-h] [-d] [-v]\n");
    fprintf(stderr, "    -h     Print this message\n");
    fprintf(stderr, "    -d     Grammar debug\n");
    fprintf(stderr, "    -v     Verbose messages\n");
}

#define BUF_SIZE 128
static char buf[BUF_SIZE];

static int
parse_callback(api_mode_arg_t *arg, void *user_data)
{
    (void)user_data;
    api_mode_show(0,arg);

    return 0;
}

int
main(int argc, char *argv[])
{
    int c;
    int flags = 0;

    while ((c = getopt(argc, argv, "sdhv")) != EOF) {
        switch (c) {
        case 's':
            flags |= SCAN_DEBUG;
            break;
        case 'd':
            flags |= PARSE_DEBUG;
            break;
        case 'v':
            flags |= PARSE_VERBOSE;
            break;
        case 'h':
        default:
            usage();
            exit(0);
            break;
        }
    }

    for (;;) {
        if (flags & PARSE_VERBOSE) {
            puts("Reading...\n");
        }
        memset(buf, 0, sizeof(buf));
        if ((fgets(buf, sizeof(buf), stdin)) == NULL) {
            break;
        }
        if (flags & PARSE_VERBOSE) {
            puts("Parsing...\n");
        }
        api_mode_parse_string(buf, flags, parse_callback, NULL);
    }
    return 0;
}

#endif /* APIMODE_INCLUDE_MAIN */
