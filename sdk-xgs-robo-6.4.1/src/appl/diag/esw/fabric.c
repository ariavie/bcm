/*
 * $Id: fabric.c,v 1.12 Broadcom SDK $
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
 * Fabric and Higig related CLI commands
 */

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/dport.h>
#include <shared/bsl.h>
#include <bcm/error.h>
#include <bcm/stack.h>
#include <bcm/debug.h>

#ifdef BCM_XGS_SUPPORT
#include <soc/higig.h>
#endif

#ifdef	BCM_XGS_SUPPORT
#define PACK_LONG(buf, val) \
    do {                                               \
        uint32 v2;                                     \
        v2 = bcm_htonl(val);                           \
        sal_memcpy(buf, &v2, sizeof(uint32));          \
    } while (0)

char if_h2higig_usage[] =
    "H2Higig [<word0> <word1> <word2>]:  Convert hex to higig info\n";

cmd_result_t
if_h2higig(int unit, args_t *args)
{
    uint32 val;
    char *arg;
    soc_higig_hdr_t hghdr;
    int i;

    sal_memset(&hghdr, 0, sizeof(soc_higig_hdr_t));
    for (i = 0; i < 3; i++) {
        if ((arg = ARG_GET(args)) == NULL) {
            break;
        }

        val = sal_strtoul(arg, NULL, 0);
        PACK_LONG(&hghdr.overlay0.bytes[i * 4], val);
    }

    soc_higig_dump(unit, "", &hghdr);

    return CMD_OK;
}

#ifdef BCM_HIGIG2_SUPPORT

char if_h2higig2_usage[] =
    "H2Higig2 [<word0> <word1> <word2> <word3> [<word4>]]:  Convert hex to higig2 info\n";

cmd_result_t
if_h2higig2(int unit, args_t *args)
{
    uint32 val;
    char *arg;
    soc_higig2_hdr_t hghdr;
    int i;

    sal_memset(&hghdr, 0, sizeof(soc_higig2_hdr_t));
    for (i = 0; i < 4; i++) {
        if ((arg = ARG_GET(args)) == NULL) {
            break;
        }

        val = sal_strtoul(arg, NULL, 0);
        PACK_LONG(&hghdr.overlay0.bytes[i * 4], val);
    }

    soc_higig2_dump(unit, "", &hghdr);

    /* Decode extension header if present */
    if ((arg = ARG_GET(args)) != NULL) {
        int eh_type;
        val = sal_strtoul(arg, NULL, 0);
        eh_type = val >> 28;
        cli_out("0x%08x <EHT=%d", val, eh_type);
        if (eh_type == 0) {
            cli_out(" TM=%d", (val >> 24) & 0x1);
            cli_out(" SEG_SEL=%d", (val >> 21) & 0x7);
            cli_out(" TAG_TYPE=%d", (val >> 19) & 0x3);
            cli_out(" QTAG=0x%04x", val & 0xffff);
        }
        cli_out(">\n");
    }

    return CMD_OK;
}
#endif /* BCM_HIGIG2_SUPPORT */

#endif	/* BCM_XGS_SUPPORT */

int _bcm_diag_fabric_not_empty;
