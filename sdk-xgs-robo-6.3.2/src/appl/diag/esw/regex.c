/*
 * $Id: regex.c 1.24 Broadcom SDK $
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
 * File:        txrx.c
 * Purpose:
 * Requires:    
 */

#ifdef INCLUDE_REGEX
#include <sal/types.h>
#include <sal/core/time.h>
#include <sal/core/thread.h>
#include <sal/core/libc.h>
#include <sal/appl/sal.h>
#include <sal/appl/io.h>

#include <soc/types.h>
#include <soc/debug.h>

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/in.h>
#define atoi _shr_ctoi 
#else
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

#include <bcm/port.h>
#include <bcm/error.h>

#include <appl/diag/system.h>
#include <bcm/bregex.h>

#ifndef NO_FILEIO 
char cmd_regex_usage[] = 
"Parameters\n"
"\tfile -- load the patterns from file\n";

typedef struct xd_s {  
    pbmp_t      pbmp; 
    char        *xd_file;
} xd_t;

#define MAX_PATTERN_PER_ENGINE 128

typedef struct diag_regex_engine_info_s {
    char                filename[256];
    int                 num_match;
    char                app_name[MAX_PATTERN_PER_ENGINE][64];
    bcm_regex_match_t   match[MAX_PATTERN_PER_ENGINE];
    bcm_regex_engine_t  engid;
    struct diag_regex_engine_info_s *next;
} diag_regex_engine_info_t;

static diag_regex_engine_info_t *engine_info_list[BCM_MAX_NUM_UNITS];
#define MAX_ENGINE_PER_UNIT     64
static bcm_regex_engine_t re_engines[BCM_MAX_NUM_UNITS][MAX_ENGINE_PER_UNIT];
static int engine_count[BCM_MAX_NUM_UNITS];
static int unit_inited[BCM_MAX_NUM_UNITS];
static int re_inited = 0;

static int regex_cmd_init(void)
{
    if (re_inited) {
        return 0;
    }
    
    sal_memset(engine_info_list, 0, sizeof(diag_regex_engine_info_t*)*BCM_MAX_NUM_UNITS);
    sal_memset(engine_count, 0, sizeof(int)*BCM_MAX_NUM_UNITS);
    re_inited = 1;
    return 0;
}

static int initialize_regex(int unit, int init)
{
    bcm_regex_config_t  recfg;
    int i, rv, port;
    soc_pbmp_t  pbmp_temp;
    
    /* Set default configuration */
    recfg.flags = BCM_REGEX_CONFIG_ENABLE | BCM_REGEX_CONFIG_IP4 |
                  BCM_REGEX_CONFIG_IP6 | BCM_REGEX_REPORT_MATCHED;
    recfg.max_flows = -1; /* max flows */
    recfg.payload_depth = -1;
    recfg.inspect_num_pkt = -1;
    recfg.inactivity_timeout_usec = 100;
    recfg.report_flags = BCM_REGEX_REPORT_NEW | BCM_REGEX_REPORT_MATCHED | 
                         BCM_REGEX_REPORT_END;
    recfg.dst_mac[0] = 0x00;
    for (i=1;i<6;i++) {
        recfg.dst_mac[i] = 0xa0 + i;
    }
    recfg.src_mac[0] = 0x00;
    for (i=1;i<6;i++) {
        recfg.src_mac[i] = 0xb0 + i;
    }
    recfg.ethertype = 0x0531;
    rv = bcm_regex_config_set(unit, &recfg);
    if (rv) {
        return CMD_FAIL;
    }

    /* Enable regex on the ports */
    BCM_PBMP_ASSIGN(pbmp_temp, PBMP_GE_ALL(unit));
    BCM_PBMP_ITER(pbmp_temp, port) {
        bcm_port_control_set(unit, port, bcmPortControlRegex, init ? 1 : 0);
    }

    unit_inited[unit] = !!init;
    return 0;
}


static cmd_result_t
regex_parse_args(int u, args_t *a, xd_t *xd)
{
    parse_table_t       pt;
    char                *xfile = NULL;

    parse_table_init(u, &pt);

    parse_table_add(&pt, "Filename", PQ_DFL|PQ_STRING,0, &xfile,NULL);

    if (parse_arg_eq(a, &pt) < 0) {
        sal_printf("Error: Unknown option: %s\n", ARG_CUR(a));
        parse_arg_eq_done(&pt);
        return(CMD_FAIL);
    }

    if (xfile) {
        xd->xd_file = sal_strdup(xfile);
    } else {
        xd->xd_file = NULL;
    }

    parse_arg_eq_done(&pt);
    return(0);
}

static int _read_line(FILE *fp, char *b, int *len)
{
    int c;
    int l = 0;  

    c = getc(fp);
    while (!((c == EOF) || (c == '\n') || (c == '\r'))) {
        b[l++] = c;
        c = getc(fp);
    }

    *len = l;
    b[l] = '\0';
    if (l) {
        return 0;
    }

    return (c == EOF) ? -1 : 0;
}

static char* _strip_front(char *l, int len)
{
    int i=0;

    /* skip leading spaces. */
    while (i < len) {
        if ((l[i] == ' ') || (l[i] == '\t')) {
            i++;
            continue;
        }
        return &l[i];
    }
    return NULL;
}

static int decode_app_name(char* s, diag_regex_engine_info_t *app)
{
    sal_strcpy(&app->app_name[app->num_match][0], s);
    return 0;
}

static int decode_match_id(char* s, diag_regex_engine_info_t *app)
{
    app->match[app->num_match].match_id = atoi(s);
    return 0;
}

static int decode_flow_timeout(char* s, diag_regex_engine_info_t *app)
{
    app->match[app->num_match].inactivity_timeout_usec = atoi(s);
    return 0;
}

static int decode_pattern(char* s, diag_regex_engine_info_t *app)
{
    int j = 0, esc = 0;

    if (s[0] != '/') {
        return -1;
    }

    s++;
    sal_memset(app->match[app->num_match].pattern, 0, BCM_REGEX_MAX_PATTERN_SIZE);
    while (*s) {
        if (!esc && (*s == '/')) {
            return 0;
        }
        app->match[app->num_match].pattern[j] = *s;
        if (*s == '\\') {
            esc = !!esc;
        }
        j++;
        s++;
    }

    if (app->match[app->num_match].pattern[j-1] == '/') {
        app->match[app->num_match].pattern[j-1] = '\0';
        return 0;
    }

    return 1;
}

typedef struct regex_cmd_str_to_flag_s {
    char *name;
    unsigned int flag;
} regex_cmd_str_to_flag_t;

static regex_cmd_str_to_flag_t *
regex_flag_get(char *str, regex_cmd_str_to_flag_t *tbl)
{
    while (tbl->name) {
        if (!sal_strcasecmp(str, tbl->name)) {
            return tbl;
        }
        tbl++;
    }
    return NULL;
}

static int decode_flags_cmn(char* s, diag_regex_engine_info_t *app, 
                            regex_cmd_str_to_flag_t *tbl, 
                            unsigned int *pflag)
{
    char *ts;
    int done = 0;
    regex_cmd_str_to_flag_t *ps2f;

    *pflag = 0;

    ts = s;
    while (!done) {
        /* skip leading spaces */
        while (*s && ((*s == ' ') || (*s == '\t'))) {
            s++;
        }
        if (!*s) {
            return 0;
        }
        ts = s;
        while (*s && (!((*s == ' ') || (*s == ',')))) {
            s++;
        }
        if (!*s) {
            done = 1;
        } else {
            *s = '\0';
            s++;
        }
        ps2f = regex_flag_get(ts, tbl);
        if (!ps2f) {
            printk("unknown regex action: %s\n", ts);
            return -1;
        }
        *pflag |= ps2f->flag;
    }
    return 0;
}

static int decode_enums(char* s, diag_regex_engine_info_t *app, 
                        regex_cmd_str_to_flag_t *tbl, 
                        unsigned int *value)
{
    char *ts;
    int done = 0;
    regex_cmd_str_to_flag_t *ps2f;

    ts = s;
    while (!done) {
        /* skip leading spaces */
        while (*s && ((*s == ' ') || (*s == '\t'))) {
            s++;
        }
        if (!*s) {
            return 0;
        }
        ts = s;
        while (*s && (!((*s == ' ') || (*s == ',')))) {
            s++;
        }
        if (!*s) {
            done = 1;
        } else {
            *s = '\0';
            s++;
        }
        ps2f = regex_flag_get(ts, tbl);
        if (!ps2f) {
            printk("unknown regex action: %s\n", ts);
            return -1;
        }
        *value = ps2f->flag;
    }
    return 0;
}
static regex_cmd_str_to_flag_t flow_options[] = {
    { "to_server", BCM_REGEX_MATCH_TO_SERVER },
    { "to_client", BCM_REGEX_MATCH_TO_CLIENT },
    { "case", BCM_REGEX_MATCH_CASE_INSENSITIVE },
    { "http", BCM_REGEX_MATCH_MULTI_FLOW },
    { "none", 0 },
    { NULL, 0 }
};

static int decode_flowopt(char* s, diag_regex_engine_info_t *app)
{
    int rv;
    
    rv = decode_flags_cmn( s, app, flow_options, 
                            &app->match[app->num_match].flags);
    if (rv) {
        return CMD_FAIL;
    }
    return 0;
}

static regex_cmd_str_to_flag_t flow_sequence[] = {
    { "first", 0},
    { "any", -1 },
    { NULL, 0 }
};

static int decode_flow_sequence(char* s, diag_regex_engine_info_t *app)
{
    unsigned int val = 0;
    
    decode_enums(s, app, flow_sequence, &val);
    app->match[app->num_match].sequence = val;
    return 0;
}

static regex_cmd_str_to_flag_t match_actions[] = {
    { "ignore", BCM_REGEX_MATCH_ACTION_IGNORE },
    { "to_cpu", BCM_REGEX_MATCH_ACTION_COPY_TO_CPU },
    { "drop", BCM_REGEX_MATCH_ACTION_DROP },
    { "none", 0 },
    { NULL, 0 }
};

static int decode_action(char* s, diag_regex_engine_info_t *app)
{
    int rv;
    
    rv = decode_flags_cmn( s, app, match_actions, 
                            &app->match[app->num_match].action_flags);
    if (rv) {
        return CMD_FAIL;
    }
    return 0;
}

static int decode_previous_tag(char* s, diag_regex_engine_info_t *app)
{
    app->match[app->num_match].requires = atoi(s);
    return 0;
}

static int decode_next_tag(char* s, diag_regex_engine_info_t *app)
{
    app->match[app->num_match].provides = atoi(s);
    return 0;
}

typedef int (*decode_tag_value)(char *buf, diag_regex_engine_info_t *app);

typedef struct regex_xml_data_s {
    char            *tag_name;
    decode_tag_value decode_value;
} regex_xml_data_t;

static regex_xml_data_t regex_match_info[] = {
    { "engine", NULL},
    { "signature", NULL},
    { "application", decode_app_name },
    { "matchid", decode_match_id },
    { "pattern", decode_pattern },
    { "flowopt", decode_flowopt },
    { "sequence", decode_flow_sequence },
    { "timeout", decode_flow_timeout},
    { "previous_match_tag", decode_previous_tag },
    { "next_match_tag", decode_next_tag },
    { "action", decode_action},
    { NULL, NULL },
};

static regex_xml_data_t *regex_cmd_get_xml_tag(char *name)
{
    regex_xml_data_t *ptag = regex_match_info;
    char *s = name;

    while(*s) {
        if (*s == '>') {
            *s = '\0';
        }
        s++;
    }

    while (ptag->tag_name) {
        if (!sal_strcasecmp(name, ptag->tag_name)) {
            return ptag;
        }
        ptag++;
    }
    return NULL;
}

/* each input line should be in the format:
 *  /pattern/id/action/
 */
static int _parse_a_xml_line(char *l, int len, 
                             diag_regex_engine_info_t *einfo,
                             regex_xml_data_t **pptag, int *tag_start)
{
    char *b;
    regex_xml_data_t *ptag;

    b = _strip_front(l, len);
    if (!b) { 
        return 0;
    }

    /* check if the line is commented */
    if (b[0] == '#') {
        return 0; /* line is commenetd */
    }

    if (b[0] == '<') {
        b++;
        *tag_start = 1;
        if (*b == '/') {
            *tag_start = 0;
            b++;
        }
        ptag = regex_cmd_get_xml_tag(b);
        if (!ptag) {
            printk("Unknown XML tag: %s\n", b);
            return -1;
        }
        *pptag = ptag;
        return 0;
    }

    ptag = *pptag;
    if (!ptag) {
        printk("Invalid tag");
        return -1;
    }

    /* decode the line accouding to provided tag */
    if (ptag->decode_value && (ptag->decode_value(b, einfo))) {
        printk("Failed to decode the line: %s\n", b);
        return -1;
    }

    return 0;
}

static cmd_result_t
cmd_create_regex_engine(int unit, args_t *a)
{
    FILE                *fp;
    char                line[512];
    int                 llen, rv, nume=0;
    xd_t                xd;
    diag_regex_engine_info_t    einfo[32], *papp;
    bcm_regex_match_t   match[64];
    regex_xml_data_t *ptag = NULL;
    int              tstart = 0, i, e, line_num = 0;
    bcm_regex_engine_t  reid;
    bcm_regex_engine_config_t eng_cfg;

    if (!SOC_UNIT_VALID(unit)) {
        return (CMD_FAIL);
    }
    sal_memset(&xd, 0, sizeof(xd_t));
    if (CMD_OK != (rv = regex_parse_args(unit, a, &xd))) {
        return(rv);
    }

    if (!xd.xd_file) {
        return CMD_FAIL;
    }

    if ((fp = sal_fopen(xd.xd_file, "r")) == NULL) {
        printk("Failed to open file: %s\n", xd.xd_file);
        return CMD_FAIL;
    }

    sal_memset(einfo, 0, sizeof(diag_regex_engine_info_t)*32);
    while(1) {
        /* parse the regex patterns from the file */
        line_num++;
        if (_read_line(fp, line, &llen)) {
            break;
        }
        if (_parse_a_xml_line(line, llen, &einfo[nume], &ptag, &tstart)) {
            printk("Error on line : %d\n", line_num);
            return CMD_FAIL;
        }
        if (ptag && (!sal_strcasecmp(ptag->tag_name, "signature")) &&
            tstart == 0) {
            sal_strcpy(einfo[nume].filename, xd.xd_file);
            einfo[nume].num_match++;
            ptag = NULL;
            tstart = 1;
        }
        if (ptag && (!sal_strcasecmp(ptag->tag_name, "engine")) &&
            tstart == 0) {
            nume++;
            ptag = NULL;
            tstart = 1;
        }
    }

    if (unit_inited[unit] == 0) {
        initialize_regex(unit, 1);
    }

    /* create an engine */
    eng_cfg.flags = 0;
    for (e=0; e<nume; e++) {
        rv = bcm_regex_engine_create(unit, &eng_cfg, &reid);
        if (rv) {
            printk("Failed to create regex engine : %d", rv);
            return CMD_FAIL;
        }

        re_engines[unit][engine_count[unit]++] = reid;

        /* Install the patterens */
        for (i = 0; i < einfo[e].num_match; i++) {
            sal_memcpy(&match[i], &einfo[e].match[i], sizeof(bcm_regex_match_t));
        }
        rv = bcm_regex_match_set(unit, reid, match, einfo[e].num_match);
        if (rv) {
            printk("Failed to install patterns : %d", rv);
            bcm_regex_engine_destroy(unit, reid);
            engine_count[unit]--;
            return CMD_FAIL;
        }

        einfo[e].engid = reid;

        /* store the match objects */
        for (i = 0; i< einfo[e].num_match; i++) {
            papp = sal_alloc(sizeof(diag_regex_engine_info_t), "einfo info");
            sal_memcpy(papp, &einfo[e], sizeof(diag_regex_engine_info_t));
            papp->engid = reid;
            papp->next = NULL;
            /* add to list  to corelate the event notifications */
            papp->next = engine_info_list[unit];
            engine_info_list[unit] = papp;
        }
    }

    if (xd.xd_file) {
        sal_free(xd.xd_file);
    }
    return (CMD_OK);
}

static int 
_regex_diag_remove_engine_from_db(int unit, bcm_regex_engine_t engid)
{
  int i, j;
    diag_regex_engine_info_t **ppapp, *papp;

    for (i = 0; i < engine_count[unit]; i++) {
        if (re_engines[unit][i] == engid) {
            for (j = i+1; j<engine_count[unit]; j++) {
                re_engines[unit][j-1] = re_engines[unit][j];
            }
            engine_count[unit]--;

            /* remove from applist */
            ppapp = &engine_info_list[unit];
            while(*ppapp) {
                papp = *ppapp;
                if (papp->engid == engid) {
                    *ppapp = papp->next;
                    sal_free(papp);
                    return 0;
                }
                ppapp = &(*ppapp)->next;
            }
        }
    }
    return -1;
}

static int 
_regex_destroy_engine_with_id(int unit, bcm_regex_engine_t engid,
                              bcm_regex_engine_config_t *config,
                              void *user_data)
{
    int rv;
    bcm_regex_engine_t del_id = (bcm_regex_engine_t)user_data;

    if ((del_id == -1) || (del_id == engid)) {
        rv = bcm_regex_engine_destroy(unit, engid);
        if (rv) {
            printk("Failed to destroy engine: %d\n", engid);
            return -1;
        }
        _regex_diag_remove_engine_from_db(unit, engid);
    }
    return CMD_OK;
}

static cmd_result_t
cmd_destroy_regex_engine(int unit, int engidx)
{
    int eng_id = -1, rv;

    if (engidx >= 0) {
        if (engidx >= engine_count[unit]) {
            printk("Invalid engine index. Not found\n");
            return CMD_FAIL;
        }
        eng_id = re_engines[unit][engidx];
    }
   
    rv = bcm_regex_engine_traverse(unit, 
                                  _regex_destroy_engine_with_id, (void*)eng_id);
    if (engine_count[unit] == 0) {
        initialize_regex(unit, 0);
    }
    return rv ? CMD_FAIL : CMD_OK;
}

static int verbose = 1;
static void bcm_regex_diag_report_cb(int unit, bcm_regex_report_t *report, 
                                    void *user_data)
{
    if (!verbose) {
        return;
    }
    
    printk("\n-----------------------------------\n");
    if (report->flags & BCM_REGEX_REPORT_NEW) {
        printk("New Flow \n");
    } else if (report->flags & BCM_REGEX_REPORT_END) {
        printk("Flow termination\n");
    } else if (report->flags & BCM_REGEX_REPORT_MATCHED) {
        printk("Flow match\n");
    }
    printk("matchID   = %d\n", report->match_id);
    printk("flow flag = %02x\n", report->flags);
    printk("protocol  = %d\n", report->protocol);
    printk("src_port  = %d\n", report->src_port);
    printk("src_ip    = %02d.%02d.%02d.%02d\n", 
                      (report->sip >> 24) & 0xff,
                      (report->sip >> 16) & 0xff,
                      (report->sip >> 8) & 0xff,
                      (report->sip >> 0) & 0xff);
    printk("src_port = %d\n", report->src_port);
    printk("dst_ip   = %02d.%02d.%02d.%02d\n", 
                      (report->dip >> 24) & 0xff,
                      (report->dip >> 16) & 0xff,
                      (report->dip >> 8) & 0xff,
                      (report->dip >> 0) & 0xff);
    return;
}

static int remon_registered = 0;

static cmd_result_t
cmd_regex_monitor(int unit, int start)
{
    int rv = 0;

    if (start) {
        if (!remon_registered) {
            rv = bcm_regex_report_register(unit,
                                   (BCM_REGEX_REPORT_NEW | \
                                    BCM_REGEX_REPORT_MATCHED | \
                                    BCM_REGEX_REPORT_END),
                                    bcm_regex_diag_report_cb, NULL);
            if (rv) {
                printk("Error(%d) Starting ReMon\n", rv);
            } else {
                remon_registered = 1;
            }
        }
    } else {
            rv = bcm_regex_report_unregister(unit,
                                   (BCM_REGEX_REPORT_NEW | \
                                    BCM_REGEX_REPORT_MATCHED | \
                                    BCM_REGEX_REPORT_END),
                                    bcm_regex_diag_report_cb, NULL);
            remon_registered = 0;
    }
    return CMD_OK;
}

static int regex_cmd_engine_dump(int unit, 
                                 bcm_regex_engine_t engine, 
                                 bcm_regex_engine_config_t *config, 
                                 void *user_data)
{
    printk("------------------------------------------\n");
    printk("\tEngine-ID: 0x%x\n", engine);
    if (config->flags & BCM_REGEX_ENGINE_CONFIG_MULTI_PACKET) {
        printk("\tMode : Multipacket");
    }
    printk("------------------------------------------\n");
    return 0;
}

static cmd_result_t dump_regex(int unit)
{
    bcm_regex_config_t recfg;

    if (bcm_regex_config_get(unit, &recfg)) {
        return CMD_FAIL;
    }

    /* display configuration */
    if (recfg.flags & BCM_REGEX_CONFIG_ENABLE) {
        printk("Regex : Enabled\n");
    } else {
        printk("Regex : Disabled\n");
        return CMD_OK;
    }

    printk("Max flow : %d\n", recfg.max_flows);
    printk("Max payload inspection size : %d\n", recfg.payload_depth);
    printk("Max packets to inspect: %d\n", recfg.inspect_num_pkt);
    printk("Report generation enabled for: ");
    if (recfg.report_flags & BCM_REGEX_REPORT_NEW) {
        printk("New Flow,");
    }
    if (recfg.report_flags & BCM_REGEX_REPORT_MATCHED) {
        printk("Flow match,");
    }
    if (recfg.report_flags & BCM_REGEX_REPORT_END) {
        printk("Flow terminated");
    }
    printk("%s\n", recfg.report_flags ? "" : "None");

    /* iterate over all the engines and display their configuration */
    bcm_regex_engine_traverse(unit, regex_cmd_engine_dump, NULL);
    return CMD_OK;
}

extern void 
_bcm_regex_sw_dump_engine(int unit, bcm_regex_engine_t engid,
                            uint32 f);
static int 
_regex_dump_engine_cb(int unit, bcm_regex_engine_t engid,
                              bcm_regex_engine_config_t *config,
                              void *user_data)
{
    bcm_regex_engine_t did = (bcm_regex_engine_t)user_data;

    if ((did == -1) || (did == engid)) {
#if defined(BCM_TRIUMPH3_SUPPORT)
        _bcm_regex_sw_dump_engine(unit, engid,0);
#else
        return CMD_NOTIMPL;
#endif
    }
    return CMD_OK;
}

static int dump_regex_engine(int unit, int engidx)
{
    int eng_id = -1;

    if (engidx >= 0) {
        if (engidx >= engine_count[unit]) {
            printk("Invalid engine index. Not found\n");
            return CMD_FAIL;
        }
        eng_id = re_engines[unit][engidx];
    }
    bcm_regex_engine_traverse(unit,_regex_dump_engine_cb, (void*)eng_id);
    return CMD_OK;
}

static int 
_regex_dump_engine_dfa_cb(int unit, bcm_regex_engine_t engid,
                              bcm_regex_engine_config_t *config,
                              void *user_data)
{
    bcm_regex_engine_t did = (bcm_regex_engine_t)user_data;

    if ((did == -1) || (did == engid)) {
#if defined(BCM_TRIUMPH3_SUPPORT)
        _bcm_regex_sw_dump_engine(unit, engid,1);
#else
        return CMD_NOTIMPL;
#endif
    }
    return CMD_OK;
}

static int dump_regex_engine_dfa(int unit, int engidx)
{
    int eng_id = -1;

    if (engidx >= 0) {
        if (engidx >= engine_count[unit]) {
            printk("Invalid engine index. Not found\n");
            return CMD_FAIL;
        }
        eng_id = re_engines[unit][engidx];
    }
    bcm_regex_engine_traverse(unit,_regex_dump_engine_dfa_cb,(void*)eng_id);
    return CMD_OK;
}

typedef struct re_diag_ud_s {
    bcm_regex_engine_t engid;
    int found;
} re_diag_ud_t;

static int
_regex_engine_present_in_hw(int unit, bcm_regex_engine_t engid,
                              bcm_regex_engine_config_t *config,
                              void *user_data)
{
    re_diag_ud_t *ud = (re_diag_ud_t*) user_data;

    if (ud->engid == engid) {
        ud->found = 1;
    }

    return CMD_OK;
}

static int _regex_engine_sync(int unit, bcm_regex_engine_t engid)
{
    re_diag_ud_t ud;

    ud.engid = engid;
    ud.found = 0;
    bcm_regex_engine_traverse(unit, _regex_engine_present_in_hw, (void*)&ud);
    if (ud.found == 0) {
        _regex_diag_remove_engine_from_db(unit, engid);
    }
    return 0;
}

static int _regex_dump_app_info(int unit, bcm_regex_engine_t engid)
{
    diag_regex_engine_info_t *papp;
    int i;

    /* remove from applist */
    papp = engine_info_list[unit];
    while(papp) {
        if (papp->engid == engid) {
            printk("\t File: %s\n", papp->filename);
            printk("\t Number of patterns : %d\n", papp->num_match);
            for (i = 0; i < papp->num_match; i++) {
                printk("\t Pattern %d : %s\n", i, papp->match[i].pattern);
            }
            return 0;
        }
        papp = papp->next;
    }
    return -1;
}

cmd_result_t cmd_regex(int unit, args_t *a)
{
    char		*subcmd, *param;
    int                 mon_start = 0, eindex, i;

    if (re_inited == 0) {
        regex_cmd_init();
    }

    if ((subcmd = ARG_GET(a)) == NULL) {
        return dump_regex(unit);
    }

    if (sal_strcasecmp(subcmd, "create") == 0) {
        return cmd_create_regex_engine(unit, a);
    } else if (sal_strcasecmp(subcmd, "destroy") == 0) {
        eindex = -1;
        if ((subcmd = ARG_GET(a))) {
            eindex = atoi(subcmd);
        } else {
            eindex = -1;
        }
        return cmd_destroy_regex_engine(unit, eindex);
    } else if (sal_strcasecmp(subcmd, "monitor") == 0) {
        if ((subcmd = ARG_GET(a)) == NULL) {
	    mon_start = 1;
        } else {
            if (sal_strcasecmp(subcmd, "start") == 0) {
                mon_start = 1;
            } else if (sal_strcasecmp(subcmd, "stop") == 0) {
                mon_start = 0;
            } else if (sal_strcasecmp(subcmd, "verbose") == 0) {
                verbose = 1;
                return 0;
            } else if (sal_strcasecmp(subcmd, "silent") == 0) {
                verbose = 0;
                return 0;
            } else {
                mon_start = 0;
            }
        }
        return cmd_regex_monitor(unit, mon_start);
    } else if (sal_strcasecmp(subcmd, "list") == 0) {
        for (i = 0; i < engine_count[unit]; i++) {
            _regex_engine_sync(unit, re_engines[unit][i]);
        }
        /* list all the engines on this unit */
        printk("Total %d engines installed.\n", engine_count[unit]);
        for (i = 0; i < engine_count[unit]; i++) {
            printk("-----engine %d --------\n", i);
            _regex_dump_app_info(unit, re_engines[unit][i]);
        }
        return CMD_OK;
    } else if (sal_strcasecmp(subcmd, "show") == 0) {
        subcmd = ARG_GET(a);
        if (!subcmd) {
            return dump_regex(unit);
        }

        param = ARG_GET(a);
        if (sal_strcasecmp(subcmd, "engine") == 0) {
            eindex = -1;
            if (param) {
                eindex = atoi(param);
            }
            return dump_regex_engine(unit, eindex);
        } else if (sal_strcasecmp(subcmd, "dfa") == 0) {
            eindex = -1;
            if (param) {
                eindex = atoi(param);
            }
            return dump_regex_engine_dfa(unit, eindex);
        }
        return CMD_OK;
    }
    return CMD_USAGE;
}
#endif 

#else
int _diag_esw_regex_not_empty;
#endif

