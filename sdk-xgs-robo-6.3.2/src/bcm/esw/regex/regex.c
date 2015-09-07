/*
 * $Id: regex.c 1.35 Broadcom SDK $
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
 * File:       bregex.c
 * Purpose:    Regex compiler
 */

#if 0
#define STANDALONE_MODE 1
#endif

#ifdef STANDALONE_MODE
#define INCLUDE_REGEX
#else
#define BROADCOM_SDK
#endif

#define OPTIMIZE_PATTERN    1

/*
 * Enable DFA minimization after computation of DFA from NFA. This is 
 * highly recommeneded as it leads to compressing/optimizing the DFA
 * to in most cases to 2-4% or the orginal DFA.
 */
#define DFA_MINIMIZE            1

/*
 * Disable nextline character to not be matched when '.' Dot character
 * is encountered in the regex pattern.
 */
#define OPT_DONT_MATCH_NL_ON_DOT    0

#ifdef BROADCOM_SDK
/* Enabling the dump of final DFA. */
#define ENABLE_FINAL_DFA_DUMP   0

/* Enable the dump of DFA before minimization step. */
#define ENABLE_DFA_DUMP_BEFORE_MINIMIZATION 0

/* enable the dump of DFA states as the list of NFA state list. */
#define DUMP_DFA_STATE          0

/* dump the symbol classes that represent the pattern. */
#define DUMP_SYMBOL_CLASSES     0

#define ENABLE_NFA_DUMP         0

/* dump regular expression in postfix format */
#define ENABLE_POST_DUMP        0

#else

/* Enabling the dump of final DFA. */
#define ENABLE_FINAL_DFA_DUMP                   1

/* Enable the dump of DFA before minimization step. */
#define ENABLE_DFA_DUMP_BEFORE_MINIMIZATION     0

/* enable the dump of DFA states as the list of NFA state list. */
#define DUMP_DFA_STATE                          0

/* dump the symbol classes that represent the pattern. */
#define DUMP_SYMBOL_CLASSES                     0

#define ENABLE_NFA_DUMP                         0

/* dump regular expression in postfix format */
#define ENABLE_POST_DUMP        0

#endif

#if defined(INCLUDE_REGEX)

#ifdef BROADCOM_SDK
#include <soc/defs.h>
#include <shared/alloc.h>
#include <sal/core/libc.h>
#include <sal/core/sync.h>
#include <sal/appl/io.h>
#include <soc/debug.h>
#include <bcm/error.h>
#include <bcm/debug.h>
#include <bcm_int/regex_api.h>
#define RE_SAL_TOI(s1,base) sal_ctoi((s1),NULL)
#define RE_SAL_DPRINT(args)  BCM_DEBUG(BCM_DBG_REGEX, args) 

#else

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "regex_api.h"

#define MEM_PROFILE     0

#if MEM_PROFILE==1
#define sal_alloc(s,t)  re_alloc_wrap((s),(t))
#define sal_free(p)   re_free_wrap((p))
#else
#define sal_alloc(s,t)  malloc((s))
#define sal_free(p)   free((p))
#endif

#define sal_strcpy(s1,s2) strcpy((s1),(s2))
#define RE_SAL_TOI(s1,base) strtol((s1),NULL, (base))
#define RE_SAL_DPRINT(p)   printf p
#define _shr_sort(a,b,s,n)    qsort((void*)(a),(b),(s),(n))
#define sal_strlen   strlen
#define sal_memset   memset
#define sal_memcpy   memcpy
#define sal_strdup   strdup
#define sal_memcmp   memcmp

#if MEM_PROFILE==1

static int peak_alloc = 0;
static int total_alloc = 0;

typedef struct _mem_prof_s {
    struct _mem_prof_s *next;
    void *buf;
    char loc[32];
    int sz;
} _mem_prof_t;

static _mem_prof_t *alloc_list = NULL;

static void *re_alloc_wrap(int size, char *tag)
{
    void *b;
    _mem_prof_t *mp;

    b = malloc(size+sizeof(_mem_prof_t));
    if (b) {
         mp = (_mem_prof_t*)b;
         mp->sz = size;
         mp->next = alloc_list;
         alloc_list = mp;
         sal_strcpy(mp->loc, tag);
         mp->buf = b + sizeof(_mem_prof_t);
         b += sizeof(_mem_prof_t);
         total_alloc += size;
         if (total_alloc > peak_alloc) {
            peak_alloc = total_alloc;
         }
    }
    return b;
}

static void re_free_wrap(void *b)
{
    _mem_prof_t **p = &alloc_list, *tmp;

    while(*p) {
        if (b == (*p)->buf) {
            tmp = *p;
            *p = tmp->next;
            total_alloc -= tmp->sz;
            free(tmp);
            break;
        } else {
            p = &(*p)->next;
        }
    }
    return;
}

static void re_dump_mem_leak(void)
{
    int total = 0;
    _mem_prof_t *mp = alloc_list;

    while(mp) {
        total += mp->sz;
        RE_SAL_DPRINT(("leak at :%s\n", mp->loc));
        mp = mp->next;
    }

    RE_SAL_DPRINT(("Total memory leak : %d bytes\n", total));
    if (peak_alloc > 0x100000) {
        RE_SAL_DPRINT(("Total Peak alloc  : %d MB \n", peak_alloc/0x100000));
    } else {
        RE_SAL_DPRINT(("Total Peak alloc  : %d KB\n", peak_alloc/1024));
    }
    return;
}

#endif /* MEM_PROFILE */

#endif

typedef struct re_wc {
    unsigned  char ctrl;
    unsigned  char c;
} re_wc;

#define POST_EN_DATA    0x01
#define POST_EN_META    0x02

#define POST_ENTRY(p)   *(p)

#define POST_EN_IS_META(p)  ((p)->ctrl == POST_EN_META)

#define WSTR_DATA_VALID(pws)  \
    (((pws)->ctrl == POST_EN_META) && ((pws)->c == '\0') ? 0 : 1)

#define WSTR_GET_DATA(pws)  (pws)->c

#define WSTR_GET_DATA_AT_OFF(pws, off)  WSTR_GET_DATA((pws)+(off))

#define WSTR_PTR_INC(pws,i)   (pws) += (i)

static int wstrlen(re_wc *s)
{
    int i = 0;

    while(WSTR_DATA_VALID(s)) {
        i++;
        WSTR_PTR_INC(s,1);
    }
    return i;
}

static void add_data_cmn(char c, re_wc **o, int *ofst, int *pmax,
                          unsigned char dtype) 
{
    re_wc *tmp, *pwc; 
    int  max = *pmax, cur_sz, new_sz;

    if (*ofst == max) {
        cur_sz = max * sizeof(re_wc);
        new_sz = max*2*sizeof(re_wc);
        tmp = sal_alloc(new_sz,"re_wc1");
        sal_memset(tmp, 0, cur_sz);
        sal_memcpy(tmp, *o, cur_sz);
        sal_free(*o);
        *o = tmp;
        *pmax = max * 2;
    }

    /* first add the data type and then data */
    pwc = *o + *ofst;
    pwc->ctrl = dtype;
    pwc->c = c;
    *ofst = *ofst + 1;
    return;
}

#define ADD_DATA(c, po, of, m)  \
        add_data_cmn((c), (po), (of), (m), POST_EN_DATA)

#define ADD_META_DATA(c, po, of, m)  \
        add_data_cmn((c), (po), (of), (m), POST_EN_META)

static int is_metachar(char c)
{
    if ((c == '(') || (c == '.') || (c == ')') || (c == '+') || 
        (c == '*') || (c == '|') || (c == '{') || (c == '}') ||
        (c == '?') || (c == '\\')) {
        return 1;
    }
    return 0;
}

#define INFINITE -1

static int parse_digit_from_range(re_wc **pb, char *mc)
{
    /* re_wc *s; */
    re_wc *b;
    int   d, ti = 0;
    char  tmp[12];

    b = *pb;
    WSTR_PTR_INC(b, 1);

    /* skip whitespaces */
    while (WSTR_DATA_VALID(b) && 
           ((WSTR_GET_DATA(b) < '0') || (WSTR_GET_DATA(b) > '9')) && 
           (WSTR_GET_DATA(b) != ',') && (WSTR_GET_DATA(b) != '}')) {
        WSTR_PTR_INC(b,1);
    }

    /* s = b;*/
    while (WSTR_DATA_VALID(b) && 
            ((WSTR_GET_DATA(b) >= '0') && (WSTR_GET_DATA(b) <= '9'))) {
        tmp[ti++] = WSTR_GET_DATA(b);
        WSTR_PTR_INC(b,1);
    }
    tmp[ti] = '\0';

    if (ti > 0) {
        d = RE_SAL_TOI(tmp,10);
    } else {
        d = INFINITE;
    }

    /* eat everything upto sep */
    *mc = '\0';
    while (WSTR_DATA_VALID(b) && 
            (WSTR_GET_DATA(b) != ',') && (WSTR_GET_DATA(b) != '}')) {
        WSTR_PTR_INC(b,1);
    }
    *mc = WSTR_GET_DATA(b);
    *pb = b;
    return d;
}

static char *classed="dDwWsSnrt.";

typedef struct re_class_info {
    char        c;
    re_wc       *exp;
    int         (*make_class_str)(int);
    unsigned int  flags;
} re_class_info;

static const unsigned int digit_tbl[] = {
        0x00000000, 
        0x03ff0000, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000
};

static const unsigned int wtbl[] = {
        0x00000000, 
        0x00000000, 
        0x07fffffe, 
        0x07fffffe, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000
};

static const unsigned int spacetbl[] = {
        0x00003e00, 
        0x00000001, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000
};

static const unsigned int isupper[] = {
        0x00000000, 
        0x00000000, 
        0x07fffffe, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000
};

static const unsigned int islower[] = {
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x07fffffe, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000
};

static const unsigned int isprintable[] = {
        0x00000000,
        0xffffffff,
        0xffffffff,
        0x7fffffff,
        0x00000000,
        0x00000000,
        0x00000000,
        0x00000000
};

#if DUMP_SYMBOL_CLASSES == 1
static int printable(unsigned char c)
{
    if (isprintable[c/32] & (1 << (c % 32))) {
        return 1;
    }
    return 0;
}
#endif

static int re_tolower(int c)
{
    c = c & 0xff;

    return (isupper[c/32] & (1 << (c%32))) ? c + 32 : c;
}

static int re_toupper(int c)
{
    c = c & 0xff;

    return (islower[c/32] & (1 << (c%32))) ? c - 32 : c;
}

static int re_isdigit(int c)
{
    c = c & 0xff;
    return (digit_tbl[c/32] & (1 << (c%32))) ? 1 : 0;
}

static int iswordc(int c)
{
    c = c & 0xff;
    return (wtbl[c/32] & (1 << (c%32))) ? 1 : 0;
}

static int re_isspace(int c)
{
    c = c & 0xff;
    return (spacetbl[c/32] & (1 << (c%32))) ? 1 : 0;
}


static int isnextline(int i)
{
    if (i == 0xa) {
        return 1;
    }
    return 0;
}
static int isreturn(int i)
{
    if (i == 0xd) {
        return 1;
    }
    return 0;
}
static int istab(int i)
{
    if (i == 0x9) {
        return 1;
    }
    return 0;
}

static int dot(int i)
{
#if OPT_DONT_MATCH_NL_ON_DOT == 1
    if (i == 0xa) {
        return 0;
    }
#endif
    return 1;
}

static re_class_info cinfo[] = {
    /* \d class */
    {
        'd', NULL, re_isdigit, 1
    },
    /* \D class */
    {
        'D', NULL, re_isdigit, 0
    },
    /* \w class */
    {
        'w', NULL, iswordc, 1
    },
    /* \W class */
    {
        'W', NULL, iswordc, 0
    },
    /* \s class */
    {
        's', NULL, re_isspace, 1
    },
    /* \s class */
    {
        'S', NULL, re_isspace, 0
    },
    {
        'n', NULL, isnextline, 1
    },
    {
        'r', NULL, isreturn, 1
    },
    {
        't', NULL, istab, 1
    },
    {
        '.', NULL, dot, 1
    },
};

static re_wc *get_class_bmap(char c, unsigned int *bmap)
{
    int idx, r;
    char *pc = classed;
    re_class_info *pcl;
    int i;

    while(*pc && (*pc != c)) {
        pc++;
    }
    if (!*pc) {
        RE_SAL_DPRINT(("Failed to find class info : %c\n", c));
        return NULL;
    }
    idx = pc - classed;
    pcl = &cinfo[idx];

    for (i = 0; i < 8; i++) {
        bmap[i] = 0;
    }

    for(i=0; i<256; i++) {
        r = !!pcl->make_class_str(i);
        if (r == pcl->flags) {
            bmap[i/32] |= (1 << (i % 32));
        }
    }
        /*RE_SAL_DPRINT(("string for class %c : %s\n", c, s)); */
    return 0;
}

static int _regex_decode_hex(char *pre, unsigned int *val)
{
    char tmp[5], *re = pre;
    int d;

    tmp[0] = '0';
    tmp[1] = 'x';
    tmp[2] = *re;
    tmp[3] = *(re+1);
    tmp[4] = '\0';
    d = RE_SAL_TOI(tmp,16);
    *val = (unsigned int ) d;
    return 0;
}

static int 
re_make_char_class_bmap(char **pre, unsigned int *bmap, int ignore_neg,
                        unsigned int re_flag)
{
    unsigned int lc = 0, tc, d;
    int  esc = 0, neg = 0;
    int i, range = 0, j, oct[4];
    int lcuc = (re_flag & BCM_TR3_REGEX_CFLAG_EXPAND_LCUC) ? 1 : 0;
    char *re = *pre, *start;
    char bmap_tmp[32];

    if (*re++ != '[') {
        return -1;
    }
    start = re;
    if (*re == '^') {
        if (!ignore_neg) {
            neg = 1;
        }
        re++;
        start = re;
    }

    sal_memset(bmap, 0, sizeof(unsigned int)*8);
    sal_memset(bmap_tmp, 0, 32);

    for(; *re && !(*re == ']' && !esc); re++) {
        if ((*re == '-') && !esc && (re != start)) {
            range = 1;
            continue;
        }
        if (!esc && (*re == '\\')) {
            esc = 0;
            switch(*(re+1)) {
                case 'n':
                    tc = 0xa;
                    re++;
                break;
                case 'r':
                    tc = 0xd;
                    re++;
                break;
                case 't':
                    tc = 9;
                    re++;
                break;
                case 'u':
                    re += 2; 
                case 'x':
                    re += 1;
                    if (_regex_decode_hex(re+1, &d)) {
                        return -1;
                    }
                    re += 2;
                    tc = d;
                    break;
                default:
                    if ((*re >= '0') && (*re <= '9')) {
                        /* octal */
                        j = 0;
                        while (*re && (*re >= '0') && (*re <= '9') && (j < 2)) {
                            oct[j++] = *re++;
                        }
                        re--;
                        tc = 0;
                        while(j > 0) {
                            tc = (tc*8) + (oct[j-1] - '0');
                            j--;
                        }
                    } else {
                        esc = 1;
                    }
                break;
            }
            if (esc) {
                continue;
            }
        } else {
            tc = *re;
        }
        if (!range) {
            lc = tc;
        }
        /* add range from lc to *re */
        for (i=lc; i<=tc;i++) {
            bmap_tmp[i/8] |= 1 << (i % 8);
        }
        esc = 0;
        range = 0;
    }
    *pre = re;

    for (i = 0; i < 256; i++) {
        if (!!(bmap_tmp[i/8] & (1 << (i % 8))) == neg) {
            continue;
        }
        bmap[i/32] |= 1 << (i % 32);
        if (lcuc && (wtbl[i/32] & (1 << (i % 32)))) {
            tc = i + ((i >= 'a') ? -32 : 32 );
            bmap[tc/32] |= 1 << (tc % 32);
        }
    }
    return 0;
}

#define DEFINE_STACK(nt,t)    \
struct stack_of_##nt {    \
    t    *pstack;  \
    t    *stack;   \
};

#define DECLARE_STACK(nt)   struct stack_of_##nt

#define STACK_INIT(s,sz,sn)  \
    (s)->pstack = (s)->stack = sal_alloc(sizeof(*(s)->stack)*sz, (sn))

#define STACK_RESET(s)  (s)->pstack = (s)->stack

#define STACK_VALID(s)  ((s)->stack ? 1 : 0)

#define STACK_DEINIT(s)     if ((s)->stack) sal_free((s)->stack)

#define STACK_PUSH(s,p)     *(s)->pstack++ = (p)

#define STACK_POP(s)      *--(s)->pstack

#define STACK_EMPTY(s) ((s)->pstack == (s)->stack)

typedef struct re_tr_grp_s {
    int num_tr;
    unsigned int trmap[8];
} re_tr_grp;

typedef struct re_tr_cls_s {
    int             id;
    int             num_tr;
    unsigned int    trmap[8];
} re_tr_cls;

typedef struct re_membuf_blk_s {
    void    *buf;
    int     size;
    int     free_ofst;
    int     ensize;
    struct  re_membuf_blk_s *next;
} re_membuf_blk_t;

struct re_dfa_state_t;

typedef struct avn_s {
    int balance;
    struct re_dfa_state_t *data;
    struct avn_s *ds[2];
} avn_t;

typedef struct re_dfa_state_t re_dfa_state_t;
typedef struct re_dfa_t
{
    re_dfa_state_t **s;
    int n;
    int sz;
    re_tr_cls *clsarr;
    int num_cls;
    re_membuf_blk_t *nbuf;
    re_membuf_blk_t *avbuf;
    avn_t   *avtree;
} re_dfa_t;

/*
 * Represents an NFA state plus zero or one or two arrows exiting.
 * if c == Match, no arrows out; matching state.
 * If c == Split, unlabeled arrows to out and out1 (if != NULL).
 * If c < 256, labeled arrow with character c to out.
 */
enum
{
    Split = 256,
    Match = 257
};

typedef struct re_nfa_state_t re_nfa_state_t;
struct re_nfa_state_t
{
    int c;
    re_nfa_state_t *out;
    re_nfa_state_t *out1;
    int       id;
    int lastlist;
};

#define RE_NFA_STATES(l)        (l)->s

#define RE_NFA_LIST_LENGTH(l)   (l)->n

#define RE_NFA_STATE_IN_LIST(l,i) (l)->s[(i)]

DEFINE_STACK(nfa_states, re_nfa_state_t *);

static int smap_bsz;

typedef struct re_nfa_list_t
{
    int n;
    unsigned int *smap;
} re_nfa_list_t;

#define NFA_SMAP_BSIZE(n)    ((n)->smap_bsz)

#define NFA_SMAP_WSIZE(n)    (((n)->nstate+31)/32)

#define NFA_LIST_RESET(nfa,l)   \
{   \
    (l)->n = 0; \
    sal_memset((l)->smap, 0, NFA_SMAP_BSIZE(nfa)); \
}

typedef struct re_nfa_t {
    re_nfa_state_t *root;
    int             smap_bsz;
    re_nfa_state_t **smap;
    unsigned int    *cmap[256];
    int nstate;
    int smap_sz;
    re_membuf_blk_t *nbuf;
    DECLARE_STACK(nfa_states) *stk;
} re_nfa_t;

static int nfa_free(re_nfa_t *ns);

#define NFA_STATE_BLOCK_SIZE  1000

static re_membuf_blk_t* 
_alloc_membuf_block(re_membuf_blk_t **plist, int count, int ensize)
{
    int sz;
    re_membuf_blk_t *pb;

    sz = sizeof(re_membuf_blk_t) + (ensize * count);
    pb = sal_alloc(sz,"membuf");
    sal_memset(pb, 0, sz);
    pb->buf = (void*) (pb+1);
    pb->free_ofst = 0;
    pb->ensize = ensize;
    pb->size = count;
    
    pb->next = *plist;
    *plist = pb;
    return pb;
}

#define NFA_BLOCK_SIZE 16300

static void *
_alloc_space_for_nfa_state_list(re_dfa_t *pdfa, int llen)
{
    int alloc;
    re_membuf_blk_t *pb;
    void *b;

    pb = pdfa->nbuf;
    alloc = ((pb == NULL) || ((pb->size - (pb->free_ofst+1)) < llen));
    if (alloc) {
        pb = _alloc_membuf_block(&pdfa->nbuf, 
                                 NFA_BLOCK_SIZE, sizeof(unsigned int));
    }

    b = pb->buf + sizeof(unsigned int)*pb->free_ofst;
    pb->free_ofst += llen;
    return b;
}

static avn_t*
_alloc_space_for_avltree_nodes(re_dfa_t *pdfa, int count, int llen)
{
    int alloc;
    re_membuf_blk_t *pb;
    avn_t *b;

    pb = pdfa->avbuf;
    alloc = ((pb == NULL) || ((pb->size - (pb->free_ofst+1)) < llen));
    if (alloc) {
        pb = _alloc_membuf_block(&pdfa->avbuf, count, sizeof(avn_t));
    }

    b = pb->buf + sizeof(avn_t)*pb->free_ofst;
    pb->free_ofst += llen;
    return b;
}

#define DFA_STATE_BLOCK_SIZE  1024

static re_nfa_state_t*
_alloc_space_for_nfa_nodes(re_nfa_t *pnfa, int count, int llen)
{
    int alloc;
    re_membuf_blk_t *pb;
    re_nfa_state_t *b;

    pb = pnfa->nbuf;
    alloc = ((pb == NULL) || ((pb->size - (pb->free_ofst+1)) < llen));
    if (alloc) {
        pb = _alloc_membuf_block(&pnfa->nbuf, count, sizeof(re_nfa_state_t));
    }

    b = pb->buf + sizeof(re_nfa_state_t)*pb->free_ofst;
    pb->free_ofst += llen;
    return b;
}

static void _free_buf_blocks(re_membuf_blk_t *nb)
{
    re_membuf_blk_t *t;

    while(nb) {
        t = nb->next;
        sal_free(nb);
        nb = t;
    }
}

static int listid;

/*
 * Represents a DFA state: a cached NFA state list.
 */
struct re_dfa_state_t
{
    re_nfa_list_t l;
    unsigned int flags;
    int     *tlist;
};

#define RE_DFA_STATE_ARCS(d)    (d)->tlist

/*
 * In order to conserve space, flags fild is encoded and contains the 
 * following in encoded fashion.
 *   - state_id
 *   - final
 *   - marked
 *   ..
 */
#define RE_DFA_STATEID(d)       ((d)->flags & 0x7ffff)
#define RE_DFA_SET_STATEID(d,s)  \
                (d)->flags = ((d)->flags & 0xfff80000) | ((s) & 0x7ffff)

#define RE_DFA_IS_FINAL(d)      ((((d)->flags >> 19) & 0x1ff) > 0)
#define RE_DFA_FINAL(d)         (((d)->flags >> 19) & 0x1ff)
#define RE_DFA_SET_FINAL(d,m)   \
            (d)->flags = ((d)->flags & 0xf007ffff) | (((m) & 0x1ff) << 19)

#define RE_DFA_IS_MARKED(d)      (((d)->flags >> 28) & 0x1)
#define RE_DFA_SET_MARKED(d,m)     \
             (d)->flags = ((d)->flags & 0xefffffff) | (((m) & 0x1) << 28)

#define RE_DFA_IS_ADDED_TO_LIST(d)  (((d)->flags >> 29) & 0x1)
#define RE_DFA_ADD_TO_LIST(d)   (d)->flags |= (1 << 29)

#define RE_DFA_SET_BLOCK_MSTR(d) (d)->flags |= (1 << 30)

#define RE_CONNECT_DFA_STATES(d1,d2,c) (d1)->tlist[(c)] = RE_DFA_STATEID(d2)

#define DFA_BLOCK_SIZE  1024

static re_dfa_state_t* _alloc_dfa_state_block(re_dfa_state_t**tbl,
                                              int num_cls)
{
    re_dfa_state_t *ps;
    int *tlist = NULL, szs, szt, i, count;

    count = DFA_BLOCK_SIZE;
    szs = sizeof(re_dfa_state_t)*count;
    ps = sal_alloc(szs,"dfasb");
    if (!ps) {
        goto error;
    }

    /* allocate transition list and attach to states */
    szt = sizeof(int)*count*num_cls;
    tlist = sal_alloc(szt,"dfalst");
    if (!tlist) {
        goto error;
    }

    sal_memset(ps, 0, szs);
    sal_memset(tlist, 0xff, szt);

    for (i = 0; i < count; i++) {
        tbl[i] = &ps[i];
        ps[i].tlist = tlist;
        tlist += num_cls;
    }

    RE_DFA_SET_BLOCK_MSTR(&ps[0]);

    return ps;

error:
    if (ps) {
        sal_free(ps);
    }

    if (tlist) {
        sal_free(tlist);
    }
    return NULL;
}

static re_tr_grp *_add_to_tr_grp_list(re_tr_grp **top, re_tr_grp *this)
{
    re_tr_grp *new_grp, *ptmp;
    int matched = 0, i, j, off;

    off = -1;
    for (i=0; i < 256; i++) {
        ptmp = top[i]; 
        if (ptmp == NULL) {
            if (off == -1) {
                off = i;
            }
            continue;
        }
        matched = 1;
        /* compare the tr group */
        for (j = 0; j < 8; j++) {
            if (this->trmap[j] != ptmp->trmap[j]) {
                matched = 0;
                break;
            }
        }
        if (matched) {
            break;
        }
    }

    if (!matched) {
        new_grp = sal_alloc(sizeof(re_tr_grp),"trgrp");
        sal_memcpy(new_grp, this, sizeof(re_tr_grp));
        top[off] = new_grp;
    }
    return 0;
}

#define RE_RESET_TRMAP(ptr) sal_memset((ptr), 0, sizeof(re_tr_grp))

#define RE_ADD_TR_TO_GRP(ptr,tr)                                \
{                                                               \
    if (((ptr)->trmap[(tr)/32] & (1 << ((tr) % 32))) == 0) {    \
        (ptr)->num_tr++;                                        \
        (ptr)->trmap[(tr)/32] |= (1 << ((tr) % 32));            \
    }                                                           \
}

#if 0
static int
sort_tr_grp(void *a, void *b)
{
    re_tr_grp *pa, *pb;
    pa = *((re_tr_grp**) a);
    pb = *((re_tr_grp**) b);
    return pa->num_tr - pb->num_tr;
}
#endif

static int
sort_tr_cls(void *a, void *b)
{
    re_tr_cls *pa, *pb;
    int ma, mb, i, j;

    pa = (re_tr_cls*) a;
    pb = (re_tr_cls*) b;
    
    ma = -1;
    for (i = 0; i < 8; i++) {
        if (pa->trmap[i] == 0) {
            continue;
        }
        for (j = 0; j < 32; j++) {
            if (pa->trmap[i] & (1 << (j % 32))) {
                ma = (i*32) + j;
                break;
            }
        }
        if (ma >= 0) {
            break;
        }
    }
    mb = -1;
    for (i = 0; i < 8; i++) {
        if (pb->trmap[i] == 0) {
            continue;
        }
        for (j = 0; j < 32; j++) {
            if (pb->trmap[i] & (1 << (j % 32))) {
                mb = (i*32) + j;
                break;
            }
        }
        if (mb >= 0) {
            break;
        }
    }
    return ma - mb;
}

static int _regex_add_token_onec(re_tr_grp **top, unsigned char c)
{
    re_tr_grp curmap;

    RE_RESET_TRMAP(&curmap);
    RE_ADD_TR_TO_GRP(&curmap, c);
    _add_to_tr_grp_list(top, &curmap);
    return 0;
}

static int _regex_add_token_multi(re_tr_grp **top, unsigned int *bmap)
{
    re_tr_grp curmap;
    int i;

    RE_RESET_TRMAP(&curmap);
    for (i = 0; i < 256; i++) {
        if (bmap[i/32] & (1 << (i % 32))) {
            RE_ADD_TR_TO_GRP(&curmap, i);
        }
    }
    _add_to_tr_grp_list(top, &curmap);
    return 0;
}

static int 
re_add_single_class(re_tr_cls *pclsarr, int num_cls, unsigned int c,
                                  re_wc **o, int *used, int *sz)
{
    int i, clid;

    for (i = 0; i < num_cls; i++) {
        #if 0
        if (pclsarr[i].num_tr != 1) {
            continue;
        }
        #endif
        if (pclsarr[i].trmap[c/32] & (1 << (c % 32))) {
            clid = pclsarr[i].id;
            if (is_metachar(clid)) {
                ADD_DATA('\\', o, used, sz);
            }
            ADD_DATA(clid, o, used, sz);
            return 0;
        }
    }
    return -1;
}

static int 
re_add_multi_class(re_tr_cls *pclsarr, int num_cls, 
                      unsigned int *bmap, re_wc **o, int *used, int *sz)
{
    int i, j, clid, first = 1, matched;

    ADD_DATA('(', o, used, sz);
    for (i = 0; i < num_cls; i++) {
        for (j = 0; j < 8; j++) {
            matched = 0;
            if (pclsarr[i].trmap[j] & (bmap[j])) {
                matched = 1;
            }
            if (matched) {
                clid = pclsarr[i].id;
                if (!first) {
                    ADD_DATA('|', o, used, sz);
                }
                if (is_metachar(clid)) {
                    ADD_DATA('\\', o, used, sz);
                }
                ADD_DATA(clid, o, used, sz);
                first = 0;
                break;
            }
        }
    }
    ADD_DATA(')', o, used, sz);
    return 0;
}

#ifdef OPTIMIZE_PATTERN
/*
 * Optimize the patterns. Some of the optimizations are:
 *  remove trailing .*
 */
static int
re_optimize_patterns(char **patterns, int num_pattern)
{
    int p, esc = 0;
    char *mark = NULL, *re;
    return 0;
    for(p=0; p <num_pattern; p++) {
        re = patterns[p];
        while(*re) {
            if (*re == '\\') {
                esc = !!esc;
            } else if (!esc && (*re == '.')) {
                if (*(re + 1) == '+') {
                    re++;
                    mark = re;
                    re++;
                } else if (*(re+1) == '*') {
                    mark = re;
                    re++;
                }
            } else {
                mark = NULL;
            }
            re++;
        }
        if (mark) {
            *mark = '\0';
        }
    }
    return 0;
}
#endif /* OPTIMIZE_PATTERN */

static int
re_case_adjust(char **patterns, int num_pattern, unsigned int *re_flags)
{
    int pi, esc = 0;
    char *re;

    for (pi = 0; pi < num_pattern; pi++) {
        if ((re_flags[pi] & (BCM_TR3_REGEX_CFLAG_EXPAND_UC |
                       BCM_TR3_REGEX_CFLAG_EXPAND_LC)) == 0) {
            continue;
        }
        re = patterns[pi];
        while(*re) {
            if (*re == '\\') {
                esc = !esc;
                re++;
                continue;
            }
            if (esc) {
                esc = 0;
                re++;
                continue;
            }
            if (re_flags[pi] & BCM_TR3_REGEX_CFLAG_EXPAND_UC) {
                if ((*re >= 97) && (*re <= 122)) {
                    *re = *re - 32;
                }
            } else if (re_flags[pi] & BCM_TR3_REGEX_CFLAG_EXPAND_LC) {
                if ((*re >= 65) && (*re <= 90)) {
                    *re = *re + 32;
                }
            }
            re++;
        }
    }
    return 0;
}

/*
 * decompose the transitions into unique clasees. The idea it to 
 * extract the least common individual transitions which can 
 * be uniquely represented. Say for example if the pattern has
 * is /abc/, it can be represented by character a, b and c. But if
 * the pattern is /a.c/, in this case '.' will be represented as
 * (\0|\1|\2|...\254|\255), it not only makes the pattern big, also
 * the possible symbols in the patterns are 256. Whereas this can be 
 * represented as 3 symbols, ie. S1 ==> {a}, S2 ==> {b} and 
 *  S3 =={anything not a and b}, so we can now represent the same patetrn
 * /a.c/ as /S1(S1|S2|S3)S2/ which makes the symbol table size to just 3
 * instead of 256.
 */
static int
re_make_symbol_classes_from_pattern(char **patterns, int num_pattern, 
                            unsigned int *re_flag,
                            re_tr_cls **pclsarr, int *num_cls)
{
    int   cl_num = 0, i, j, k, count, p, esc = 0, rv = 0;
    unsigned int bmap[8], oct[4], u;
    re_tr_grp *trlist[256];
    re_tr_cls *clsarr = NULL, *cls;
    char *re;
#if DUMP_SYMBOL_CLASSES == 1
    int lc = -1;
#endif
   
    sal_memset(trlist, 0, sizeof(re_tr_grp*)*256);

    for(p=0; p <num_pattern; p++) {
        re = patterns[p];
        while(*re) {
            switch(*re) {
                case '\\':
                    if (!esc) {
                        esc = 1;
                    } else {
                        _regex_add_token_onec(trlist, *re);
                    }
                    break;
                case '[':
                    if (esc) {
                        _regex_add_token_onec(trlist, *re);
                        esc = 0;
                    } else {
                        re_make_char_class_bmap(&re, bmap, 1, 
                                                re_flag ? re_flag[p] : 0);
                        _regex_add_token_multi(trlist, bmap);
                    }
                    break;
                case '{':
                    if (esc) {
                        _regex_add_token_onec(trlist, *re);
                        esc = 0;
                    } else {
                        while (*re != '}') {
                            re++;
                        }
                    }
                    break;
                default:
                    if (!esc) {
                        if (!((*re == '(') || (*re == ')') || 
                             (*re == '|') || (*re == '*') || (*re == '+'))) {
                            if (*re == '.') {
                                #if 0
                                get_class_bmap(*re, bmap);
                                _regex_add_token_multi(trlist, bmap);
                                #endif
                            } else {
                                if ((re_flag) && 
                                    (re_flag[p] & BCM_TR3_REGEX_CFLAG_EXPAND_LCUC) &&
                                    (re_tolower(*re) != re_toupper(*re))) {
                                    _regex_add_token_onec(trlist, re_tolower(*re));
                                    _regex_add_token_onec(trlist, re_toupper(*re));
                                } else {
                                    _regex_add_token_onec(trlist, *re);
                                }
                            }
                        }
                    } else {
                        switch(*re) {
                            case 'd':
                            case 'D':
                            case 'w':
                            case 'W':
                            case 's':
                            case 'S':
                            case 'n':
                            case 'r':
                            case 't':
                                get_class_bmap(*re, bmap);
                                _regex_add_token_multi(trlist, bmap);
                                break;
                            case 'u':
                                re += 2;
                            case 'x':
                                if (_regex_decode_hex(re+1, &u)) {
                                    rv = -1;
                                    goto fail;
                                }
                                re += 2;
                                _regex_add_token_onec(trlist, (char)u);
                                break;
                            default:
                                if ((*re >= '0') && (*re <= '9')) {
                                    /* octal */
                                    j = 0;
                                    while (*re && (*re >= '0') && (*re <= '9') && (j < 2)) {
                                        oct[j++] = *re++;
                                    }
                                    re--;
                                    i = 0;
                                    while(j > 0) {
                                        i = (i*8) + (oct[j-1] - '0');
                                        j--;
                                    }
                                    _regex_add_token_onec(trlist, i);
                                } else {
                                    _regex_add_token_onec(trlist, *re);
                                }
                                break;
                        }
                        esc = 0;
                    }
                    break;
            }
            re++;
        }
    }

#if OPT_DONT_MATCH_NL_ON_DOT == 1
    _regex_add_token_onec(trlist, 0xa);
#endif

    sal_memset(bmap, 0xff, sizeof(unsigned int)*8);
    for (i = 0; trlist[i] && (i < 256); i++) {
        for (j = 0; j < 8; j++) {
            bmap[j] &= ~trlist[i]->trmap[j];
        }
    }

    for (j = 0; j < 8; j++) {
        if (bmap[j]) {
            _regex_add_token_multi(trlist, bmap);
            break;
        }
    }

    /* count the number of diferent transitions/groups/classes */
    for (count=0, j = 0; trlist[j] && (j < 256); j++) {
        count++;
    }


    for (i = 0; i < 256; i++) {
        k = 0;
        for (j = 0; j < count; j++) {
            if (trlist[j] == NULL) {
                continue;
            }
            if (trlist[j]->trmap[i/32] & (1 << (i % 32))) {
                k++;
            }
            if (k >= 2) {
                break;
            }
        }
        if (k >= 2) {
            for (j = 0; j < count; j++) {
                if (trlist[j] == NULL) {
                    continue;
                }
                if (trlist[j]->trmap[i/32] & (1 << (i % 32))) {
                    if (trlist[j]->num_tr == 1) {
                        sal_free(trlist[j]);
                        trlist[j] =  NULL;
                    } else {
                        trlist[j]->trmap[i/32] &= ~(1 << (i % 32));
                        trlist[j]->num_tr--;
                    }
                }
            }
            _regex_add_token_onec(trlist, i);
            count++;
        }
    }

    /* compact and remove any nulls */
    for (count = 0, i = 0; i < 255; i++) {
        if ((trlist[i] == NULL) && (trlist[i+1])) {
            trlist[i] = trlist[i+1];
            trlist[i+1] = NULL;
        }
        if (trlist[i]) {
            count++;
        }
    }

    clsarr = sal_alloc(sizeof(re_tr_cls)*256,"clsarr");
    sal_memset(clsarr, 0, sizeof(re_tr_cls)*256);
    cl_num = 0;
    for (i = 0; i < count; i++) {
        cls = &clsarr[cl_num];
        cls->num_tr = 0;
        for (j = 0; j < 256; j++) {
            if (trlist[i]->trmap[j/32] & (1 << (j % 32))) {
                cls->trmap[j/32] |= 1 << ( j % 32 );
                cls->num_tr++;
            }
        }
        if (cls->num_tr) {
            cl_num++;
        } else {
            i = 0;
        }
    }

    _shr_sort(clsarr, count, sizeof(re_tr_cls), sort_tr_cls);

    for (i = 0; i < cl_num; i++) {
        clsarr[i].id = i;
    }

#if DUMP_SYMBOL_CLASSES == 1
    for (i = 0; i < cl_num; i++) {
        cls = &clsarr[i];
        lc = -1;
        RE_SAL_DPRINT(("\nClass %d: ", cls->id));
        for (j = 0; j < 256; j++) {
            if (cls->trmap[j/32] & (1 << (j % 32))) {
                if (lc == -1) {
                    lc = j;
                }
            } else {
                if (lc >= 0) {
                    RE_SAL_DPRINT(("["));
                    if (printable(lc)) {
                        RE_SAL_DPRINT(("%c", lc));
                    } else {
                        RE_SAL_DPRINT(("\\%d", lc));
                    }
                    RE_SAL_DPRINT(("-"));
                    if (printable(j - 1)) {
                        RE_SAL_DPRINT(("%c", j - 1));
                    } else {
                        RE_SAL_DPRINT(("\\%d", j - 1));
                    }
                    RE_SAL_DPRINT(("]"));
                    lc = -1;
                }
            }
        }
        if (lc >= 0)  {
            RE_SAL_DPRINT(("[\%d-\\255]", lc));
        }
        RE_SAL_DPRINT(("\n"));
        
    }
#endif
   
    *pclsarr = clsarr;
    *num_cls  = cl_num;

fail:
    /* free up trlists */
    for (i = 0; i < 256; i++) {
        if (trlist[i]) {
            sal_free(trlist[i]);
        }
    }
    if (rv) {
        *num_cls = 0;
        *pclsarr = NULL;
        sal_free(clsarr);
    }
    return rv;
}

#if ENABLE_POST_DUMP==1
static void dump_post(re_wc *postfix, int detail)
{
    re_wc *p;

    for(p=postfix; WSTR_DATA_VALID(p); WSTR_PTR_INC(p,1)) {
        RE_SAL_DPRINT(("%s%d", POST_EN_IS_META(p) ? "m" : "c", p->c));
    }
    RE_SAL_DPRINT(("\n"));
}
#endif

static re_wc* 
re_convert_tokens_to_class(char *re, unsigned int re_flag, 
                                re_tr_cls *pclsarr, int num_cl)
{
    re_wc *o;
    int   l, used = 0, esc = 0, m = 0, c;
    unsigned int d;
    int oct[4], j;
    unsigned int bmap[8];

    l = sal_strlen(re);
    if (!l) {
        return NULL;
    }

    /* 
     * each character uses 2 bytes, since \0 is valid data, so inorder to
     * distinguish end of string with \0, we store metadata and character.
     */
    m = l + 1;
    o = sal_alloc(m*sizeof(re_wc),"re2cls");
    used = 0;

    while(*re) {
        switch(*re) {
            case '\\':
                if (!esc) {
                    esc = 1;
                } else {
                    if (re_add_single_class(pclsarr, num_cl, *re, 
                                                        &o, &used, &m)) {
                        goto fail;
                    }
                }
                break;
            case '[':
                if (esc) {
                    esc = 0;
                    if (re_add_single_class(pclsarr, num_cl, 
                                                *re, &o, &used, &m)) {
                        goto fail;
                    }
                } else {
                    re_make_char_class_bmap(&re, bmap, 0, re_flag ? re_flag : 0);
                    if (re_add_multi_class(pclsarr, num_cl, 
                                            bmap, &o, &used, &m)) {
                        goto fail;
                    }
                }
                break;
            case '{':
                if (esc) {
                    esc = 0;
                    if (re_add_single_class(pclsarr, num_cl, 
                                                *re, &o, &used, &m)) {
                        goto fail;
                    }
                } else {
                    do {
                        ADD_DATA(*re, &o, &used, &m);
                    } while (*re++ != '}');
                    continue;
                }
                break;
            default:
                if (!esc) {
                    if (*re == '.') {
                        get_class_bmap(*re, bmap);
                        if (re_add_multi_class(pclsarr, num_cl, 
                                                bmap, &o, &used, &m)) {
                            goto fail;
                        }
                    } else {
                        if (is_metachar(*re)) {
                            ADD_DATA(*re, &o, &used, &m);
                        } else {
                            if ((re_flag & BCM_TR3_REGEX_CFLAG_EXPAND_LCUC) &&
                                (re_tolower(*re) != re_toupper(*re))) {
                                ADD_DATA('(', &o, &used, &m);
                                if (re_add_single_class(pclsarr, num_cl, 
                                                 re_tolower(*re), &o, &used, &m)) {
                                    goto fail;
                                }
                                ADD_DATA('|', &o, &used, &m);
                                if (re_add_single_class(pclsarr, num_cl, 
                                                  re_toupper(*re), &o, &used, &m)) {
                                    goto fail;
                                }
                                ADD_DATA(')', &o, &used, &m);
                            } else {
                                if (re_add_single_class(pclsarr, num_cl, 
                                                            *re, &o, &used, &m)) {
                                    goto fail;
                                }
                            }
                        }
                    }
                } else {
                    switch(*re) {
                        case 'd':
                        case 'D':
                        case 'w':
                        case 'W':
                        case 's':
                        case 'S':
                        case 'n':
                        case 'r':
                        case 't':
                            get_class_bmap(*re, bmap);
                            if (re_add_multi_class(pclsarr, num_cl, 
                                                        bmap, &o, &used, &m)) {
                                goto fail;
                            }
                            break;
                        case 'u':
                            re += 2; /* ?? */
                        case 'x':
                            if (_regex_decode_hex(re+1, &d)) {
                                goto fail;
                            }
                            re += 2;
                            if (re_add_single_class(pclsarr, num_cl, d, 
                                                &o, &used, &m)) {
                                goto fail;
                            }
                            break;
                        default:
                            c = *re;
                            if ((*re >= '0') && (*re <= '9')) {
                                /* octal */
                                j = 0;
                                while (*re && (*re >= '0') && (*re <= '9') && (j < 2)) {
                                    oct[j++] = *re++;
                                }
                                re--;
                                c = 0;
                                while(j > 0) {
                                    c = (c*8) + (oct[j-1] - '0');
                                    j--;
                                }
                            }
                            if (re_add_single_class(pclsarr, num_cl, 
                                                    c, &o, &used, &m)) {
                                goto fail;
                            }
                            break;
                    }
                    esc = 0;
                }
                break;
        }
        re++;
    }

    ADD_META_DATA('\0', &o, &used, &m);

#if ENABLE_POST_DUMP==1
    RE_SAL_DPRINT(("char re\n"));
    dump_post(o, 1);
#endif

    return o;

fail:
    sal_free(o);
    return NULL;
}

static re_wc * re_preprocess(re_wc *re)
{
    int   min, max;
    int   l, used = 0, esc = 0, m, c, j, skip;
    char  mc;
    int   toklen = 0, star;
    int    ts[100], stackp, curtok_off = 0;
    re_wc *curtok = NULL, *o;

#define pusht(off) ts[stackp++] = (off)
#define popt() (stackp == 0) ? -1 : ts[--stackp]

    stackp = 0;

    l = wstrlen(re);
    if (!l) {
        return NULL;
    }
    
    m = l+1;
    o = sal_alloc(m*sizeof(re_wc),"repp");
    used = 0;

    while(WSTR_DATA_VALID(re)) {
        /* make sure we have enough room */
        switch(WSTR_GET_DATA(re)) {
            case '{':
                if (esc) {
                    ADD_DATA(WSTR_GET_DATA(re), &o, &used, &m);
                } else {
                    #if 0
                    if (!curtok) {
                        goto FAIL;
                    }
                    #endif
                    if (toklen <= 0) {
                        goto FAIL;
                    }
                    /* decode min, max */
                    min = parse_digit_from_range(&re, &mc);
                    if (min == INFINITE) {
                        goto FAIL;
                    } else if (mc == '}') {
                        max = min;
                    } else {
                        max = parse_digit_from_range(&re, &mc);
                    }
                    skip = 1;

                    /* repeat the previous token */
                    if (min) {
                        /* since token is already put in output once, do it min-1*/
                        for (c = 0; c < min; c++) {
                            if (skip) {
                                skip--;
                                continue;
                            }
                            j = 0;
                            while (j < toklen) {
                                curtok = o + curtok_off;
                                ADD_DATA(WSTR_GET_DATA_AT_OFF(curtok,j), 
                                                            &o, &used, &m);
                                j++;
                            }
                        }
                    }
                    star = 0;
                    if (max == INFINITE) {
                        max = 1;
                        star = 1;
                    } else {
                        max -= min;
                    }
                    if (max > 0) {
                        for (c = 0; c < max; c++) {
                            if (skip == 0) {
                                j = 0;
                                while (j < toklen) {
                                    curtok = o + curtok_off;
                                    ADD_DATA(WSTR_GET_DATA_AT_OFF(curtok,j), 
                                             &o, &used, &m);
                                    j++;
                                }
                            } else {
                                skip--;
                            }
                            ADD_DATA(star ? '*' : '?', &o, &used, &m);
                        }
                    } 
                    curtok = NULL;
                    toklen = 0;
                }
                break;

            case '(':
                pusht(used);
                ADD_DATA(WSTR_GET_DATA(re), &o, &used, &m);
                break;

            case ')':
                ADD_DATA(WSTR_GET_DATA(re), &o, &used, &m);
                curtok_off = popt();
                toklen = used - curtok_off;
                break;

            case '\\':
                curtok_off = used;
                ADD_DATA(WSTR_GET_DATA(re), &o, &used, &m);
                WSTR_PTR_INC(re,1);
                ADD_DATA(WSTR_GET_DATA(re), &o, &used, &m);
                toklen = 2;
                break;

            default:
                curtok_off = used;
                toklen = 1;
                ADD_DATA(WSTR_GET_DATA(re), &o, &used, &m);
                break;
        }
        WSTR_PTR_INC(re,1);
    }
    ADD_META_DATA('\0', &o, &used, &m);
    return o;
FAIL:
    sal_free(o);
    return NULL;
}

#define REPOST_STK_SIZE 100
static re_wc* re2post(re_wc *re)
{
    int nalt, natom, lmc = 0;
    int      sz, used = 0;
    re_wc *dst, *post; /* *start */
    struct {
        int nalt;
        int natom;
    } paren[REPOST_STK_SIZE], *p;

    if (!re) {
        return NULL;
    }
    /* start = re; */
    p = paren;
    sz = wstrlen(re) * 4;
    post = dst = sal_alloc(sizeof(re_wc)*sz,"repost");
    nalt = 0;
    natom = 0;
    for(; WSTR_DATA_VALID(re); WSTR_PTR_INC(re,1)) {
        switch(WSTR_GET_DATA(re)) {
            case '(':
                if (natom > 1) {
                    --natom;
                    ADD_META_DATA('.', &dst, &used, &sz);
                }
                if (p >= paren+REPOST_STK_SIZE) {
                    RE_SAL_DPRINT(("paren stack full..\n"));
                    goto fail;
                }
                p->nalt = nalt;
                p->natom = natom;
                p++;
                nalt = 0;
                natom = 0;
                lmc = 0;
                break;
            case '|':
                if (natom == 0) {
                    RE_SAL_DPRINT(("No atoms to connect..\n"));
                    goto fail;
                }
                while(--natom > 0) {
                    ADD_META_DATA('.', &dst, &used, &sz);
                }
                nalt++;
                lmc = 0;
                break;
            case ')':
                if (p == paren) {
                    RE_SAL_DPRINT(("point to paren to and yet paren found..\n"));
                    goto fail;
                }
                if (natom == 0) {
                    RE_SAL_DPRINT(("No atom in ) case....\n"));
                    return NULL;
                }
                while(--natom > 0) {
                    ADD_META_DATA('.', &dst, &used, &sz);
                }
                for(; nalt > 0; nalt--) {
                    ADD_META_DATA('|', &dst, &used, &sz);
                }
                --p;
                nalt = p->nalt;
                natom = p->natom;
                natom++;
                lmc = 0;
                break;
            case '*':
            case '+':
            case '?':
                if (lmc) {
                    RE_SAL_DPRINT(("error: meta char without atoms\n"));
                    goto fail;
                }
                if (natom == 0) {
                    RE_SAL_DPRINT(("No atom in + or * or ? case....\n"));
                    goto fail;
                }
                ADD_META_DATA(WSTR_GET_DATA(re), &dst, &used, &sz);
                lmc = 1;
                break;
            default:
                lmc = 0;
                if (natom > 1) {
                    --natom;
                    ADD_META_DATA('.', &dst, &used, &sz);
                }
                if (WSTR_GET_DATA(re) == '\\') {
                    WSTR_PTR_INC(re,1);
                }
                ADD_DATA(WSTR_GET_DATA(re), &dst, &used, &sz);
                natom++;
                break;
        }
    }
    if (p != paren) {
        return NULL;
    }
    while(--natom > 0) {
        ADD_META_DATA('.', &dst, &used, &sz);
    }
    for(; nalt > 0; nalt--) {
        ADD_META_DATA('|', &dst, &used, &sz);
    }
    ADD_META_DATA('\0', &dst, &used, &sz);

#if ENABLE_POST_DUMP==1
    dump_post(post, 1);
#endif

    return post;
    
fail:
    sal_free(dst);
    return NULL;
}


/* Allocate and initialize re_nfa_state_t */
re_nfa_state_t*
new_nfa_state(re_nfa_t *pnfa, int c, re_nfa_state_t *out, re_nfa_state_t *out1)
{
    re_nfa_state_t *s, **prev;
    int sz;

    if (pnfa->smap_sz == pnfa->nstate) {
        prev = pnfa->smap;
        sz = sizeof(re_nfa_state_t*)*(pnfa->smap_sz+NFA_STATE_BLOCK_SIZE);
        pnfa->smap = sal_alloc(sz,"nfa_stpp");
        sal_memset(pnfa->smap, 0, sz);
        if (prev) {
            sal_memcpy(pnfa->smap, prev, 
                            sizeof(re_nfa_state_t*)*pnfa->smap_sz);
            sal_free(prev);
        }

        pnfa->smap_sz += NFA_STATE_BLOCK_SIZE;
    }


    /* alloc new block for NFA states if full */
    s = _alloc_space_for_nfa_nodes(pnfa, DFA_STATE_BLOCK_SIZE, 1);
    
    s->lastlist = 0;
    s->c = c;
    s->out = out;
    s->out1 = out1;
    s->id = pnfa->nstate++;
    pnfa->smap[s->id] = s;
    return s;
}

typedef struct re_nfa_frag_t re_nfa_frag_t;
typedef union re_nfa_ptrl_t re_nfa_ptrl_t;
struct re_nfa_frag_t
{
    re_nfa_state_t *start;
    re_nfa_ptrl_t *out;
};

DEFINE_STACK(frag, re_nfa_frag_t);

/* Initialize re_nfa_frag_t struct. */
static re_nfa_frag_t frag(re_nfa_state_t *start, re_nfa_ptrl_t *out)
{
    re_nfa_frag_t n = { start, out };
    return n;
}

/*
 * Since the out pointers in the list are always 
 * uninitialized, we use the pointers themselves
 * as storage for the Ptrlists.
 */
union re_nfa_ptrl_t
{
    re_nfa_ptrl_t *next;
    re_nfa_state_t *s;
};

/* Create singleton list containing just outp. */
static re_nfa_ptrl_t* list1(re_nfa_state_t **outp)
{
    re_nfa_ptrl_t *l;

    l = (re_nfa_ptrl_t*)outp;
    l->next = NULL;
    return l;
}

/* Patch the list of states at out to point to start. */
static void patch(re_nfa_ptrl_t *l, re_nfa_state_t *s)
{
    re_nfa_ptrl_t *next;

    for(; l; l=next) {
        next = l->next;
        l->s = s;
    }
}

/* Join the two lists l1 and l2, returning the combination. */
static re_nfa_ptrl_t* ptrl_append(re_nfa_ptrl_t *l1, re_nfa_ptrl_t *l2)
{
    re_nfa_ptrl_t *oldl1;

    oldl1 = l1;
    while(l1->next) {
        l1 = l1->next;
    }
    l1->next = l2;
    return oldl1;
}

static re_nfa_state_t* post2nfa(re_nfa_t *pnfa, re_wc *postfix, int match_idx)
{
    re_wc *p;
    re_nfa_frag_t e1, e2, e;
    re_nfa_state_t *s;
    re_nfa_state_t *matchstate;
    DECLARE_STACK(frag) fstack;

    if (postfix == NULL) {
        return NULL;
    }

    STACK_INIT(&fstack, 1024, "post_stk");

    if (!STACK_VALID(&fstack)) {
        return NULL;
    }

#define push(s) STACK_PUSH(&fstack,s)
#define pop()   STACK_POP(&fstack)

    for(p=postfix; WSTR_DATA_VALID(p); WSTR_PTR_INC(p,1)) {
        if (POST_EN_IS_META(p)) {
            switch(p->c) {
                case '.':	/* catenate */
                    e2 = pop();
                    e1 = pop();
                    patch(e1.out, e2.start);
                    push(frag(e1.start, e2.out));
                    break;
                case '|':	/* alternate */
                    e2 = pop();
                    e1 = pop();
                    s = new_nfa_state(pnfa, Split, e1.start, e2.start);
                    push(frag(s, ptrl_append(e1.out, e2.out)));
                    break;
                case '?':	/* zero or one */
                    e = pop();
                    s = new_nfa_state(pnfa, Split, e.start, NULL);
                    push(frag(s, ptrl_append(e.out, list1(&s->out1))));
                    break;
                case '*':	/* zero or more */
                    e = pop();
                    s = new_nfa_state(pnfa, Split, e.start, NULL);
                    patch(e.out, s);
                    push(frag(s, list1(&s->out1)));
                    break;
                case '+':	/* one or more */
                    e = pop();
                    s = new_nfa_state(pnfa, Split, e.start, NULL);
                    patch(e.out, s);
                    push(frag(e.start, list1(&s->out1)));
                    break;
            }
        } else {
            s = new_nfa_state(pnfa, p->c, NULL, NULL);
            push(frag(s, list1(&s->out)));
        }
    }

    e = pop();
    if (!STACK_EMPTY(&fstack))  {
        
        return NULL;
    }
    STACK_DEINIT(&fstack);

    matchstate = new_nfa_state(pnfa, Match + match_idx, NULL, NULL);
    patch(e.out, matchstate);
    return e.start;
#undef pop
#undef push
}

static void all_states_connected_by_epsilon(re_nfa_t *nfa,
                            re_nfa_list_t *l, re_nfa_state_t *s);
static void step(re_nfa_t *nfa, re_nfa_list_t*, int, re_nfa_list_t*);

/* Compute initial state list */
static re_nfa_list_t* closure_0(re_nfa_t *nfa, re_nfa_state_t *start, 
                                re_nfa_list_t *l)
{
    listid++;
    NFA_LIST_RESET(nfa, l);
    all_states_connected_by_epsilon(nfa, l, start);
    return l;
}

static void all_states_connected_by_epsilon(re_nfa_t *nfa, 
                                re_nfa_list_t *nlist, re_nfa_state_t *s)
{
    DECLARE_STACK(nfa_states) *stk;

    stk = nfa->stk;

    STACK_RESET(stk);
    STACK_PUSH(stk, s);

    while(!STACK_EMPTY(stk)) {
        s = STACK_POP(stk);
        if (s->lastlist == listid) {
            continue;
        }

        if (s->c == Split) {
            STACK_PUSH(stk, s->out);
            STACK_PUSH(stk, s->out1);
            continue;
        }
        s->lastlist = listid;
        nlist->smap[s->id/32] |= (1 << (s->id % 32));
        nlist->n++;
    }
}

static void step(re_nfa_t *nfa, re_nfa_list_t *clist, 
                    int c, re_nfa_list_t *nlist)
{
    int i, j, wsz;
    re_nfa_state_t *s;
    unsigned int val, *cmap;

    listid++;
    NFA_LIST_RESET(nfa, nlist);

    cmap = nfa->cmap[c];
    wsz = NFA_SMAP_WSIZE(nfa);
    for (i=0; i<wsz; i++) {
        val = cmap[i] & clist->smap[i];
        for (j = 0; val && (j < 32); j++) {
            if ((val & (1 << j)) == 0) {
                continue;
            }
            val &= ~(1 << j);
            s = nfa->smap[(i*32)+j];
            all_states_connected_by_epsilon(nfa, nlist, s->out);
        }
    }
}

/* Compare lists: first by length, then by members. */
static int
listcmp(re_nfa_list_t *l1, re_nfa_list_t *l2)
{
    int i;

    if (l1->n < l2->n) {
        return -1;
    }
    if (l1->n > l2->n) {
        return 1;
    }

    i = sal_memcmp(l1->smap, l2->smap, smap_bsz);
    i = (i < 0) ? -1 : ((i > 0) ? 1 : 0);
    return i;
}

static re_dfa_state_t* 
_lookup_dfa_from_cache(re_dfa_t *pdfa, re_nfa_list_t *l)
{
    int i;
    avn_t *node;

    node = pdfa->avtree;
    while(node) {
        i = listcmp(&node->data->l, l);
        if (i == 0) {
            return node->data;
        } else {
            node = (i < 0) ? node->ds[1] : node->ds[0];
        }
    }
    return NULL;
}

static avn_t *rotate_single(avn_t *root, int dir)
{
    avn_t *save = root->ds[!dir];
    root->ds[!dir] = save->ds[dir];
    save->ds[dir] = root;
    return save;
}

static avn_t *rotate_double (avn_t *root, int dir)
{
    avn_t *save = root->ds[!dir]->ds[dir];

    root->ds[!dir]->ds[dir] = save->ds[!dir];
    save->ds[!dir] = root->ds[!dir];
    root->ds[!dir] = save;

    save = root->ds[!dir];
    root->ds[!dir] = save->ds[dir];
    save->ds[dir] = root;

    return save;
}

static avn_t *
_add_dfa_state_to_cache(avn_t *root, avn_t *newen, 
                        re_dfa_state_t* d, int *done)
{
    int dir, bal, diff;
    avn_t *n, *nn;

    if (!root) {
        newen->balance = 0;
        newen->data = d;
        newen->ds[0] = newen->ds[1] = NULL;
        return newen;
    } else {
        diff = listcmp(&root->data->l, &d->l);
        if (diff == 0) {
            return root;
        }
        dir = (diff < 0);
        root->ds[dir] = _add_dfa_state_to_cache(root->ds[dir], newen, d, done);
        if ( !*done ) {
            root->balance += (dir == 0) ? -1 : +1;
            if (root->balance == 0) {
                *done = 1;
            } else if ((root->balance > 1) || (root->balance < -1)) {
                n = root->ds[dir];
                bal = (dir == 0) ? -1 : 1;
                if ( n->balance == bal ) {
                    root->balance = n->balance = 0;
                    root = rotate_single(root, !dir);
                }
                else { 
                    n = root->ds[dir];
                    nn = n->ds[!dir];

                    if (nn->balance == 0) {
                        root->balance = n->balance = 0;
                    } else if (nn->balance == bal) {
                        root->balance = -bal;
                        n->balance = 0;
                    } else {
                        root->balance = 0;
                        n->balance = bal;
                    }

                    nn->balance = 0;
                    root = rotate_double(root, !dir );
                }
                *done = 1;
            }
        }
    }
    return root;
}


/*
 * Return the cached re_dfa_state_t for list l,
 * creating a new one if needed.
 */
static int state_id = 0;

static re_dfa_state_t* 
new_dfa_state(re_nfa_list_t *l, int num_cls, re_dfa_t *pdfa, re_nfa_t *nfa)
{
    int sz, done;
    re_dfa_state_t *d, **prev;
    avn_t *avn;

    /* lookup dfa state (list of nfa states) in cache if exist. */
    d = _lookup_dfa_from_cache(pdfa, l);
    if (d) {
        return d;
    }

    /* DFA states are stored as an array. If the number of entries
     * in the array is equal to array size, grwo the array */
    if (pdfa->n == pdfa->sz) {
        prev = pdfa->s;
        sz = sizeof(re_dfa_state_t*)*(pdfa->sz+DFA_BLOCK_SIZE);
        pdfa->s = sal_alloc(sz,"dfa_stpp");
        if (prev) {
            sal_memcpy(pdfa->s, prev, sizeof(re_dfa_state_t*)*pdfa->sz);
            sal_free(prev);
        }

        /* Alloc dfa state block */
        if (!_alloc_dfa_state_block(pdfa->s + pdfa->sz, num_cls)) {
            return NULL;
        }
        pdfa->sz += DFA_BLOCK_SIZE; 
    }

    d = pdfa->s[pdfa->n];
    d->l.smap = _alloc_space_for_nfa_state_list(pdfa, NFA_SMAP_WSIZE(nfa));
    sal_memcpy(d->l.smap, l->smap, NFA_SMAP_BSIZE(nfa));
    d->l.n = RE_NFA_LIST_LENGTH(l);

    /* alloc space for avl node */
    avn = _alloc_space_for_avltree_nodes(pdfa, DFA_BLOCK_SIZE, 1);
    done = 0;
    pdfa->avtree = _add_dfa_state_to_cache(pdfa->avtree, avn, d, &done);

    return d;
}

static re_dfa_state_t* 
next_dfa_state_on_transition(re_dfa_state_t *d, int c, re_nfa_list_t *l1, 
                             int num_cls, re_dfa_t *pdfa, re_nfa_t *nfa)
{
    step(nfa, &d->l, c, l1);
    return new_dfa_state(l1, num_cls, pdfa, nfa);
}

static int add_state_to_dfa(re_dfa_t *pdfa, re_dfa_state_t *d)
{
    /* check if already exist */
    if (RE_DFA_IS_ADDED_TO_LIST(d)) {
        return 0;
    }
    RE_DFA_SET_STATEID(d, pdfa->n);
    RE_DFA_ADD_TO_LIST(d);

#if DUMP_DFA_STATE == 1
    {
        int k, l, wsz;
        wsz = smap_bsz / sizeof(unsigned int);
        RE_SAL_DPRINT(("DFA State %d represents nfa states\n", pdfa->n));
        for (k = 0; k < wsz; k++) {
            if (d->l.smap[k] == 0) {
                continue;
            }
            for (l = 0; l < 32; l++) {
                if (d->l.smap[k] & (1 << l)) {
                    RE_SAL_DPRINT((" %d,", (k*32)+l));
                }
            }
        }
        RE_SAL_DPRINT(("\n"));
    }
#endif
    pdfa->n++;
    return 0;
}

static re_dfa_state_t*
get_next_unmarked_dstate(re_dfa_t *l, int skip)
{
    int ii;
    re_dfa_state_t *d;

    if (skip < 0) {
        skip = 0;
    }

    for (ii = skip; ii < l->n; ii++) {
        d = l->s[ii];
        if (!RE_DFA_IS_MARKED(d)) {
            return d;
        }
    }
    return NULL;
}

static int get_transition_list(re_nfa_t *nfa, re_dfa_state_t *d, 
                                unsigned char *trlist, int *ntr)
{
    re_nfa_state_t *s;
    int     c, i, j, wsz, ctr;
    unsigned int val;
    unsigned int trmap[8];

    ctr = *ntr = 0;
    sal_memset(trmap, 0, sizeof(unsigned int)*8);

    wsz = NFA_SMAP_WSIZE(nfa);
    for (i=0; i<wsz; i++) {
        if (d->l.smap[i] == 0) {
            continue;
        }
        val = d->l.smap[i];
        for (j = 0; val && (j < 32); j++) {
            if ((val & (1 << j)) == 0) {
                continue;
            }
            val &= ~(1 << j);
            s = nfa->smap[(i*32)+j];
            if (s->c >= Match) {
                continue;
            }
            c = s->c & 0xff;
            if ((trmap[c/32] & (1 << (c % 32))) == 0) {
                trlist[ctr++] = c;
                trmap[c/32] |= (1 << (c % 32));
            }
        }
    }
    *ntr = ctr; 
    return 0;
}

static int dfa_free(re_dfa_t *l);

static int nfa_to_dfa(re_nfa_t *nfa, re_dfa_t *pdfa, int num_cls)
{
    re_dfa_state_t *d, *d1;
    re_nfa_state_t *s;
    int     i, k, l, c, wsz, bsz, from = 0, rv = REGEX_ERROR_NONE, ntr, maid;
    unsigned int val;
    re_nfa_list_t l1, l2;
    unsigned char trlist[256];
    DECLARE_STACK(nfa_states) nfa_stk;
#if DUMP_DFA_STATE == 1
    int nlist[2048], nc;
#endif

    listid = 0;

    bsz = NFA_SMAP_BSIZE(nfa);
    l1.smap = sal_alloc(bsz,"tmp_l1");
    sal_memset(l1.smap, 0, bsz);
    l2.smap = sal_alloc(bsz, "tmp_l2");
    sal_memset(l2.smap, 0, bsz);

    STACK_INIT(&nfa_stk, 4096, "nfa_stk");
    nfa->stk = &nfa_stk;

    d = new_dfa_state(closure_0(nfa, nfa->root, &l1), num_cls, pdfa, nfa);
    if (!d) {
        rv = REGEX_ERROR_NO_MEMORY;
        goto fail;
    }
    add_state_to_dfa(pdfa, d);

    while((d = get_next_unmarked_dstate(pdfa, from++))) {
        get_transition_list(nfa, d, trlist, &ntr);
        for (i = 0; i < ntr; i++) {
            c = trlist[i];
            d1 = next_dfa_state_on_transition(d, c, &l1, num_cls, pdfa, nfa);
            add_state_to_dfa(pdfa, d1);
            RE_CONNECT_DFA_STATES(d, d1, c);
        }
        RE_DFA_SET_MARKED(d, 1);
    }

    /* update the final states distinctly to differentiate nfa fragments */
    wsz = NFA_SMAP_WSIZE(nfa);
    for (i = 0; i < pdfa->n; i++) {
        maid = -1;
#if DUMP_DFA_STATE == 1
        nc = 0;
#endif
        d = pdfa->s[i];
        for (k=0; k<wsz; k++) {
            if (d->l.smap[k] == 0) {
                continue;
            }
            val = d->l.smap[k];
            for (l = 0; val && (l < 32); l++) {
                if ((d->l.smap[k] & (1 << l)) == 0) {
                    continue;
                }
                val &= ~(1 << l);
#if DUMP_DFA_STATE == 1
                nlist[nc++] = (k*32)+l;
#endif
                s = nfa->smap[(k*32)+l];
                if ((s->c >= Match) && (maid == -1)) {
                    RE_DFA_SET_FINAL(d, (s->c - Match + 1));
                    maid = RE_DFA_FINAL(d);
                }
            }
        }
#if DUMP_DFA_STATE == 1
        if (maid != -1) {
            RE_SAL_DPRINT(("DFA State %d represents nfa states : matchid %d\n",
                            i, RE_DFA_FINAL(d)));
            for (k = 0; k < nc; k++) {
                RE_SAL_DPRINT((" %d,", nlist[k]));
            }
            RE_SAL_DPRINT(("\n"));
        }
#endif
    }

fail:
    /* free up temp nfa stack */
    STACK_DEINIT(&nfa_stk);
    /* free up temp list */
    sal_free(l1.smap);
    sal_free(l2.smap);

    if (rv < 0) {
        /* error, free up resources. */
        dfa_free(pdfa);
        pdfa = NULL;
    }
    return 0;
}

#ifdef DFA_MINIMIZE

typedef struct re_inv_dfa_s {
    int *inv_delta;
    int *inv_delta_set;
} re_inv_dfa_t;

static int create_inverted_dfa(re_dfa_t *dfa, re_inv_dfa_t *idfa)
{
    int n, c;
    int lastDelta = 0, *inv_lists, *inv_list_last;
    int s, i, j, t, go_on;

    n = dfa->n + 1;

    idfa->inv_delta = sal_alloc(sizeof(int) * n*dfa->num_cls,"invdelta");
    sal_memset(idfa->inv_delta, 0, sizeof(int) * n*dfa->num_cls);
    idfa->inv_delta_set = sal_alloc(sizeof(int) *2*n*dfa->num_cls,"inv_dset"); 
    sal_memset(idfa->inv_delta_set, 0, sizeof(int) *2*n*dfa->num_cls);

    lastDelta = 0;
    inv_lists = sal_alloc(sizeof(int)*n,"invlist");
    sal_memset(inv_lists, 0, sizeof(int)*n);
    inv_list_last = sal_alloc(sizeof(int)*n,"inv_llast");
    sal_memset(inv_list_last, 0, sizeof(int)*n);

    for (c = 0; c < dfa->num_cls; c++) {
        for (s = 0; s < n; s++) {
            inv_list_last[s] = -1;
            idfa->inv_delta[(s*dfa->num_cls) + c] = -1;
        }

        idfa->inv_delta[(0*dfa->num_cls)+c] = 0;
        inv_list_last[0] = 0;

        for (s = 1; s < n; s++) {
            t = dfa->s[s-1]->tlist[c] + 1;

            if (inv_list_last[t] == -1) {
                idfa->inv_delta[(t*dfa->num_cls) + c] = s;
                inv_list_last[t] = s;
            }
            else {
                inv_lists[inv_list_last[t]] = s;
                inv_list_last[t] = s; 
            }
        }

        for (s = 0; s < n; s++) {
            i = idfa->inv_delta[(s*dfa->num_cls) + c];
            idfa->inv_delta[(s*dfa->num_cls) + c] = lastDelta;
            j = inv_list_last[s];
            go_on = (i != -1);
            while (go_on) {
                go_on = (i != j);
                idfa->inv_delta_set[lastDelta++] = i;
                i = inv_lists[i];
            }
            idfa->inv_delta_set[lastDelta++] = -1;
        }
    } 

    sal_free(inv_lists);
    sal_free(inv_list_last);
    return 0;
}

static void free_inverted_dfa(re_inv_dfa_t *idfa)
{
    if (idfa->inv_delta) {
        sal_free(idfa->inv_delta);
    }
    if (idfa->inv_delta_set) {
        sal_free(idfa->inv_delta_set);
    }
}

typedef struct re_dm_block_cb_s {
    int *block;
    int *b_forward;
    int *b_backward;
    int num_block;
    int b0_off;
    int b_max;
} re_dm_block_cb_t;

static int create_block_list(re_dfa_t *l, re_inv_dfa_t *idfa,
                             re_dm_block_cb_t *bcb)
{
    int n = l->n + 1, s, found, t, last, b_i;

    bcb->block = sal_alloc(sizeof(int)*2*n,"blklist");
    if (bcb->block == NULL) {
        goto fail;
    }
    sal_memset(bcb->block, 0, sizeof(int)*2*n);
    bcb->b_forward = sal_alloc(sizeof(int)*2*n,"bbfwd");
    if (bcb->b_forward == NULL) {
        goto fail;
    }
    sal_memset(bcb->b_forward, 0, sizeof(int)*2*n);
    bcb->b_backward = sal_alloc(sizeof(int)*2*n,"bbbwd");
    if (bcb->b_backward == NULL) {
        goto fail;
    }
    sal_memset(bcb->b_backward, 0, sizeof(int)*2*n);

    bcb->num_block = n;
    bcb->b0_off = n; 

    bcb->b_forward[bcb->b0_off]  = 0;
    bcb->b_backward[bcb->b0_off] = 0;          
    bcb->b_forward[0]   = bcb->b0_off;
    bcb->b_backward[0]  = bcb->b0_off;
    bcb->block[0]  = bcb->b0_off;
    bcb->block[bcb->b0_off] = 1;

    for (s = 1; s < n; s++) {
        int b = bcb->b0_off+1;
        found = 0;
        while (!found && b <= bcb->num_block) {
            t = bcb->b_forward[b];

            if (RE_DFA_IS_FINAL(l->s[s-1])) {
                found = RE_DFA_IS_FINAL(l->s[t-1]) && 
                    (RE_DFA_FINAL(l->s[s-1]) == RE_DFA_FINAL(l->s[t-1]));
            }
            else {
                found = !RE_DFA_IS_FINAL(l->s[t-1]);
            }

            if (found) {
                bcb->block[s] = b;
                bcb->block[b]++;

                last = bcb->b_backward[b];
                bcb->b_forward[last] = s;
                bcb->b_forward[s] = b;

                bcb->b_backward[b] = s;
                bcb->b_backward[s] = last;
            }

            b++;
        }

        if (!found) {
            bcb->block[s] = b;
            bcb->block[b]++;

            bcb->b_forward[b] = s;
            bcb->b_forward[s] = b;
            bcb->b_backward[b] = s;
            bcb->b_backward[s] = b;

            bcb->num_block++;
        }
    } 

    bcb->b_max = bcb->b0_off;
    for (b_i = bcb->b0_off+1; b_i <= bcb->num_block; b_i++) {
        if (bcb->block[bcb->b_max] < bcb->block[b_i]) {
            bcb->b_max = b_i;
        }
    }
    return 0;
fail:
    return -1;
}

static void free_dm_block(re_dm_block_cb_t *bcb)
{
    if (bcb->block) {
        sal_free(bcb->block);
    }
    if (bcb->b_forward) {
        sal_free(bcb->b_forward);
    }
    if (bcb->b_backward) {
        sal_free(bcb->b_backward);
    }
}

static re_dfa_t * dfa_minimize(re_dfa_t *l)
{
    int numStates = l->n, n, c;
    re_dm_block_cb_t bcb;
    int *l_forward, *l_backward, anchorL;
    re_inv_dfa_t idfa;
    int numSplit, *twin, *vsd, *vd, numd;
    int s, i, j, t, last, blk_i;
    int index, indexD, indexTwin, b;
    int *trans, *move, amount, sz;
    unsigned int *kill;
    int B_j, a, min_s;

    if (l->n == 0) {
        return l;
    }

    n = numStates+1;

    sal_memset(&idfa, 0, sizeof(re_inv_dfa_t));
    if (create_inverted_dfa(l, &idfa)) {
        goto fail;
    }

    /* create blocks */
    sal_memset(&bcb, 0, sizeof(re_dm_block_cb_t));
    if (create_block_list(l, &idfa, &bcb)) {
        goto fail;
    }

    l_forward = sal_alloc(sizeof(int)*((n*l->num_cls)+1),"lfwd");
    sal_memset(l_forward, 0, sizeof(int)*((n*l->num_cls)+1));
    l_backward = sal_alloc(sizeof(int)*((n*l->num_cls)+1),"lbwd");
    sal_memset(l_backward, 0, sizeof(int)*((n*l->num_cls)+1));

    anchorL = n*l->num_cls;

    l_forward[anchorL] = anchorL;
    l_backward[anchorL] = anchorL;

    blk_i = (bcb.b_max == bcb.b0_off) ? bcb.b0_off+1 : bcb.b0_off;

    index = (blk_i - bcb.b0_off)*l->num_cls;
    while (index < (blk_i + 1 - bcb.b0_off)*l->num_cls) {
        last = l_backward[anchorL];
        l_forward[last]     = index;
        l_forward[index]    = anchorL;
        l_backward[index]   = last;
        l_backward[anchorL] = index;
        index++;
    }

    while (blk_i <= bcb.num_block) {
        if (blk_i != bcb.b_max) {
            index = (blk_i - bcb.b0_off)*l->num_cls;
            while (index < (blk_i + 1 - bcb.b0_off)*l->num_cls) {
                last = l_backward[anchorL];
                l_forward[last]     = index;
                l_forward[index]    = anchorL;
                l_backward[index]   = last;
                l_backward[anchorL] = index;
                index++;
            }
        }
        blk_i++;
    } 

    twin = sal_alloc(sizeof(int)*2*n,"twin");
    sal_memset(twin, 0, sizeof(int)*2*n);
    vsd = sal_alloc(sizeof(int)*2*n,"vsd");
    sal_memset(vsd, 0, sizeof(int)*2*n);
    vd = sal_alloc(sizeof(int)*n,"vd");
    sal_memset(vd, 0, sizeof(int)*n);

    while (l_forward[anchorL] != anchorL) {
        int B_j_a = l_forward[anchorL];      
        l_forward[anchorL] = l_forward[B_j_a];
        l_backward[l_forward[anchorL]] = anchorL;
        l_forward[B_j_a] = 0;
        B_j = bcb.b0_off + B_j_a / l->num_cls;
        a   = B_j_a % l->num_cls;

        numd = 0;
        s = bcb.b_forward[B_j];
        while (s != B_j) {
            t = idfa.inv_delta[(s*l->num_cls) + a];
            while (idfa.inv_delta_set[t] != -1) {
                vd[numd++] = idfa.inv_delta_set[t++];
            }
            s = bcb.b_forward[s];
        }      

        numSplit = 0;

        for (indexD = 0; indexD < numd; indexD++) {
            s = vd[indexD];
            blk_i = bcb.block[s];
            vsd[blk_i] = -1; 
            twin[blk_i] = 0;
        }

        for (indexD = 0; indexD < numd; indexD++) {
            s = vd[indexD];
            blk_i = bcb.block[s];

            if (vsd[blk_i] < 0) {
                vsd[blk_i] = 0;
                t = bcb.b_forward[blk_i];
                while (t != blk_i && (t != 0 || bcb.block[0] == B_j) && 
                       (t == 0 || bcb.block[l->s[t-1]->tlist[a]+1] == B_j)) {
                    vsd[blk_i]++;
                    t = bcb.b_forward[t];
                }
            }
        }

        for (indexD = 0; indexD < numd; indexD++) {
            s = vd[indexD];
            blk_i = bcb.block[s];

            if (vsd[blk_i] != bcb.block[blk_i]) {
                int B_k = twin[blk_i];
                if (B_k == 0) { 
                    B_k = ++bcb.num_block;
                    bcb.b_forward[B_k] = B_k;
                    bcb.b_backward[B_k] = B_k;

                    twin[blk_i] = B_k;

                    twin[numSplit++] = blk_i;
                }

                bcb.b_forward[bcb.b_backward[s]] = bcb.b_forward[s];
                bcb.b_backward[bcb.b_forward[s]] = bcb.b_backward[s];

                last = bcb.b_backward[B_k];
                bcb.b_forward[last] = s;
                bcb.b_forward[s] = B_k;
                bcb.b_backward[s] = last;
                bcb.b_backward[B_k] = s;

                bcb.block[s] = B_k;
                bcb.block[B_k]++;
                bcb.block[blk_i]--;

                vsd[blk_i]--; 
            }
        } 

        for (indexTwin = 0; indexTwin < numSplit; indexTwin++) {
            int blk_i = twin[indexTwin];
            int B_k = twin[blk_i];
            for (c = 0; c < l->num_cls; c++) {
                int B_i_c = (blk_i-bcb.b0_off)*l->num_cls+c;
                int B_k_c = (B_k-bcb.b0_off)*l->num_cls+c;
                if (l_forward[B_i_c] > 0) {
                    last = l_backward[anchorL];
                    l_backward[anchorL] = B_k_c;
                    l_forward[last] = B_k_c;
                    l_backward[B_k_c] = last;
                    l_forward[B_k_c] = anchorL;
                }
                else {
                    if (bcb.block[blk_i] <= bcb.block[B_k]) {
                        last = l_backward[anchorL];
                        l_backward[anchorL] = B_i_c;
                        l_forward[last] = B_i_c;
                        l_backward[B_i_c] = last;
                        l_forward[B_i_c] = anchorL;              
                    }
                    else {
                        last = l_backward[anchorL];
                        l_backward[anchorL] = B_k_c;
                        l_forward[last] = B_k_c;
                        l_backward[B_k_c] = last;
                        l_forward[B_k_c] = anchorL;              
                    }
                }
            }
        }
    }

    free_inverted_dfa(&idfa);
    sal_free(twin);
    sal_free(vsd);
    sal_free(vd);

    trans = sal_alloc(sizeof(int)*numStates,"trans");
    sal_memset(trans, 0, sizeof(int)*numStates);

    sz = ((numStates+31)/32)*sizeof(unsigned int);
    kill = sal_alloc(sz,"kill");
    sal_memset(kill, 0, sz);

    move = sal_alloc(sizeof(int)*numStates,"move");
    sal_memset(move, 0, sizeof(int)*numStates);

    for (b = bcb.b0_off+1; b <= bcb.num_block; b++) {
        s = bcb.b_forward[b];
        min_s = s;
        for (; s != b; s = bcb.b_forward[s]) {
            if (min_s > s) {
                min_s = s;
            }
        }
        min_s--; 
        for (s = bcb.b_forward[b]-1; s != b-1; s = bcb.b_forward[s+1]-1) {
            trans[s] = min_s;
            kill[s/32] |= (s != min_s) ? (1 << (s % 32)) : 0 ;
        }
    }

    free_dm_block(&bcb);
    
    sal_free(l_forward);
    sal_free(l_backward);

    amount = 0;
    sz = ((numStates+31)/32)*sizeof(unsigned int);
    for (i = 0; i < numStates; i++) {
        if (kill[i/32] & (1 << (i%32))) {
            amount++;
        } else {
            move[i] = amount;
        }
    }

    for (i = 0, j = 0; i < numStates; i++) {
        if ((kill[i/32] & (1 << (i % 32))) == 0) {
            for (c = 0; c < l->num_cls; c++) {
                if ( l->s[i]->tlist[c] >= 0 ) {
                    l->s[j]->tlist[c]  = trans[ l->s[i]->tlist[c] ];
                    l->s[j]->tlist[c] -= move[ l->s[j]->tlist[c] ];
                }
                else {
                    l->s[j]->tlist[c] = l->s[i]->tlist[c];
                }
            }

            RE_DFA_SET_FINAL(l->s[j], RE_DFA_FINAL(l->s[i]));
            j++;
        }
    }
    numStates = j;

    sal_free(trans);
    sal_free(kill);
    sal_free(move);

    /* free up unused states */
    if (j % DFA_BLOCK_SIZE) {
        j += DFA_BLOCK_SIZE - (j % DFA_BLOCK_SIZE);
    }
    for (; j < l->n; j += DFA_BLOCK_SIZE) {
        sal_free(l->s[j]->tlist);
        sal_free(l->s[j]);
    }

    l->n = numStates;
    return l;

fail:
    free_inverted_dfa(&idfa);
    free_dm_block(&bcb);
    dfa_free(l);
    return NULL;
}


#endif

#if ENABLE_NFA_DUMP == 1
static void dump_nfa(re_nfa_t *nfa, re_nfa_state_t *root)
{
    int i;
    re_nfa_state_t *s;

    RE_SAL_DPRINT(("Entry state is : %d\n", root->id));
    for (i=0; i<nfa->nstate; i++) {
        s = nfa->smap[i];
        if (s->c >= Match) {
            RE_SAL_DPRINT(("State [FINAL %d] %d:\n", (s->c - Match), s->id));
        } else {
            RE_SAL_DPRINT(("State %d:\n", s->id));
        }
        if (s->c == Split) {
            RE_SAL_DPRINT(("\t With epsilon in %d\n", s->out->id));
            RE_SAL_DPRINT(("\t With epsilon in %d\n", s->out1->id));
        } else {
            if (s->out) {
                RE_SAL_DPRINT(("\tWith %d in %d\n", s->c, s->out->id));
            }
            if (s->out1) {
                RE_SAL_DPRINT(("\tWith %d in %d\n", s->c, s->out1->id));
            }
        }
    }
}
#endif

/*
 * Create a single NFA corrsponding to all the patterns.
 * The function does the following:
 *      - convert each patter to class representation
 *      - preprocess the pattern and expand it for example
 *          \d is expanded to (0|1|2|..|8|9) etc.
 *      - convert the pattern to postfix represetation. Note 
 *          the postfix representation is not string but utilizes 2bytes
 *          to represent the character since the pattern might have \0 in
 *          between which would terminate the string.
 *      - postfix representation of the pattern is then converted ti NFA.
 *      - All the NFA are joined together using Split (epsilon transition.)
 *      - If all goes well, the final NFA is returned back to caller.
 */
static int make_nfa(char **re, unsigned int *res_flags, 
                    int num_pattern, re_nfa_t **ppnfa,
                    re_tr_cls *pclsarr, int num_cl)
{
    re_wc *post;
    re_nfa_state_t **sub_nfa, *n1, *n2, *s;
    re_nfa_t *pnfa;
    int     pattern;
    re_wc   *re1, *re2;
    int     rv = REGEX_ERROR_NONE, i;

    pnfa = sal_alloc(sizeof(re_nfa_t),"NFA");
    sal_memset(pnfa, 0, sizeof(re_nfa_t));

    /* 
     * array to store the individual NFA strands till they are all
     * combined into single final NFA.
     */
    sub_nfa = sal_alloc(sizeof(re_nfa_state_t*)*num_pattern,"sub_nfa");
    if (sub_nfa == NULL) {
        rv = REGEX_ERROR_NO_MEMORY;
        goto fail;
    }

    for (pattern=0; pattern < num_pattern; pattern++) {
        re1 = re_convert_tokens_to_class(re[pattern], 
                    res_flags ? res_flags[pattern] : 0, pclsarr, num_cl);
        if (!re1) {
            rv = REGEX_ERROR_EXPANSION_FAIL;
            goto fail;
        }
        re2 = re_preprocess(re1);
        sal_free(re1);
        if (!re2) {
            rv = REGEX_ERROR_EXPANSION_FAIL;
            goto fail;
        }

        post = re2post(re2);
        sal_free(re2);
        if (!post) {
            rv = REGEX_ERROR_NO_POST;
            goto fail;
        }
        sub_nfa[pattern] = post2nfa(pnfa, post, pattern);
#if ENABLE_NFA_DUMP == 1
        RE_SAL_DPRINT(("-------NFA for %s\n", re[pattern]));
        dump_nfa(pnfa, sub_nfa[pattern]);
#endif
        sal_free(post);
        if (sub_nfa[pattern] == NULL) {
            rv = REGEX_ERROR_EXPANSION_FAIL;
            goto fail;
        }
    }

    if (num_pattern == 1) {
        pnfa->root = sub_nfa[0];
    } else {
        for (pattern = 0; pattern < num_pattern - 1; pattern++) {
            n1 = sub_nfa[pattern];
            n2 = sub_nfa[pattern+1];
            sub_nfa[pattern+1] = new_nfa_state(pnfa, Split, n1, n2);
        }
        pnfa->root = sub_nfa[pattern];
    }

#if ENABLE_NFA_DUMP == 1
    RE_SAL_DPRINT(("--------\n\n\n--- Final NFA ---\n\n"));
    dump_nfa(pnfa, pnfa->root);
#endif

    /* 
     * store the byte size required to represent all the states in NFA. 
     * The reason to store it, the information is required in dfa 
     * computation and required like millions of times, this just 
     * optimizes computation a bit.
     */
    pnfa->smap_bsz = ((pnfa->nstate + 31)/32)*sizeof(unsigned int);
    smap_bsz = pnfa->smap_bsz;

    /*
     * create a calss map bitvector. This optimizes traversing the 
     * NFA of specified transation.
     */
    for (i=0; i<num_cl; i++) {
        pnfa->cmap[i] = sal_alloc(NFA_SMAP_BSIZE(pnfa),"nfa_cmap");
        sal_memset(pnfa->cmap[i], 0, NFA_SMAP_BSIZE(pnfa));
    }

    for (i=0; i<pnfa->nstate; i++) {
        s = pnfa->smap[i];
        if (s->c > 255) {
            continue;
        }
        if (s->c >= num_cl) {
            rv = REGEX_ERROR_EXPANSION_FAIL;
            goto fail;
        }
        pnfa->cmap[s->c][s->id/32] |= (1 << (s->id % 32));
    }

fail:
    /* free sub NFA */
    if (rv) {
        nfa_free(pnfa);
        pnfa = NULL;
    }
    sal_free(sub_nfa);

    *ppnfa = pnfa;
    return rv;
}

#if ENABLE_FINAL_DFA_DUMP == 1
regex_cb_error_t 
bcm_regex_dfa_dump(unsigned int flags, int match_idx, int in_state, 
                           int from_c, int to_c, int to_state, 
                           int num_dfa_state, void *user_data)
{
    static int last_state = -1;

    if (flags & DFA_TRAVERSE_START) {
        last_state = -1;
        return REGEX_CB_OK;
    }

    if (flags & DFA_TRAVERSE_END) {
        return REGEX_CB_OK;
    }

    /* if last state is not same as this state, insert goto IDLE state */
    if (last_state != in_state) {
        if (flags & DFA_STATE_FINAL) {
            RE_SAL_DPRINT(("State [Final /* return %d; */] %d:\n", 
                            match_idx, in_state));
        } else {
            RE_SAL_DPRINT(("State %d:\n", in_state));
        }
        last_state = in_state;
    }

    if ((from_c == -1) || (to_c == -1)) {
        return 0;
    }

    RE_SAL_DPRINT(("   %d -> %d ", in_state, to_state));
    RE_SAL_DPRINT(("["));
    RE_SAL_DPRINT(("\\%d", from_c));
    RE_SAL_DPRINT(("-"));
    RE_SAL_DPRINT(("\\%d", to_c));
    RE_SAL_DPRINT(("];\n"));
    return REGEX_CB_OK;
}
#endif

static int dfa_free(re_dfa_t *l)
{
    int i;

    if (!l) {
        return 0;
    }

    for (i = 0; i < l->n; i+= DFA_BLOCK_SIZE) {
        sal_free(l->s[i]->tlist);
        sal_free(l->s[i]);
    }

    sal_free(l->s);

    if (l->clsarr) {
        sal_free(l->clsarr);
    }

    _free_buf_blocks(l->nbuf);
    _free_buf_blocks(l->avbuf);
    
    sal_free(l);
    return 0;
}

static int
make_dfa(int num_pattern, re_dfa_t **ppdfa, re_nfa_t **ppnfa, re_tr_cls *clsarr,
         int num_cls)
{
    re_dfa_t  *pdfa;
    re_nfa_t  *pnfa = *ppnfa;
    int rv = 0;

    pdfa = sal_alloc(sizeof(re_dfa_t),"DFA");
    sal_memset(pdfa, 0, sizeof(re_dfa_t));
    pdfa->sz = 0;
    pdfa->s = NULL;
    pdfa->n = 0;
    pdfa->clsarr = clsarr;
    pdfa->num_cls = num_cls;

    rv = nfa_to_dfa(pnfa, pdfa, num_cls);
    if (rv) {
        rv = REGEX_ERROR_NO_DFA;
        goto fail;
    }

    /* free up NFA */
    nfa_free(pnfa);
    *ppnfa = NULL;

#if ENABLE_DFA_DUMP_BEFORE_MINIMIZATION == 1
    RE_SAL_DPRINT(("Total of %d state before minimization\n", pdfa->n));
    if (bcm_regex_dfa_traverse(pdfa, bcm_regex_dfa_dump, NULL)) {
        return -1;
    }
    RE_SAL_DPRINT(("-----------------------------------\n\n\n\n"));
#endif

#if DFA_MINIMIZE == 1
    RE_SAL_DPRINT(("Total of %d state before minimization\n", pdfa->n));
    pdfa = dfa_minimize(pdfa);
    if (!pdfa) {
        rv = REGEX_ERROR_NO_DFA;
        goto fail;
    }

#if ENABLE_FINAL_DFA_DUMP == 1
    RE_SAL_DPRINT(("Total of %d state after minimization\n", pdfa->n));
    RE_SAL_DPRINT(("Miniminal DFA is\n"));
    if (bcm_regex_dfa_traverse(pdfa, bcm_regex_dfa_dump, NULL)) {
        return -1;
    }
#endif
#endif

fail:
    *ppdfa = pdfa;
    
    return rv;
}

static int nfa_free(re_nfa_t *nfa)
{
    int i;

    if (nfa == NULL) {
        return 0;
    }

    for(i=0; i<256; i++) {
        if (nfa->cmap[i])
            sal_free(nfa->cmap[i]);
    }

    if (nfa->smap) {
        sal_free(nfa->smap);
    }
  
    _free_buf_blocks(nfa->nbuf);
    sal_free(nfa);
    return 0;
}

/*
 * Compiles the set of patterns into a signle DFA. If the ppdfa is
 * not NULL, computed DFA is not freed and is preserved so that
 * user/application can inspect the DFA, ie for the API layer to
 * transform the DFA to device specific HW representation of the DFA.
 *
 * Note: This function is not reenterant.
 */
int
bcm_regex_compile(char **re, unsigned int *res_flags, int num_pattern,
                  unsigned int cflags, void** ppdfa)
{
    int rv = REGEX_ERROR_NONE, num_cl = 0, p, ptlen;
    re_dfa_t *pdfa = NULL;
    re_tr_cls *pclsarr = NULL;
    re_nfa_t *pnfa = NULL;
    char **nre;

    *ppdfa = NULL;

    listid = 0;
    state_id = 0;

    if (num_pattern <= 0) {
        return 0;
    }

    nre = sal_alloc(sizeof(char*)*num_pattern,"tmp_re");
    if (nre == NULL) {
        return REGEX_ERROR_NO_MEMORY;
    }
    sal_memset(nre, 0, sizeof(char*)*num_pattern);
    
    for(p=0; p<num_pattern; p++) {
        if (re[p] == NULL) {
            continue;
        }
        nre[p] = sal_strdup(re[p]);
        ptlen = sal_strlen(nre[p]);
        if ((nre[p][ptlen - 1] == '$') && 
            ((ptlen > 1) ? (nre[p][ptlen - 2] != '\\') :  1)) {
            nre[p][ptlen - 1] = '\0';
        }
    }

#ifdef OPTIMIZE_PATTERN
    re_optimize_patterns(nre, num_pattern);
#endif

    rv = re_case_adjust(nre, num_pattern, res_flags);

    /* 
     * make the classes so that we can replace the tokens with the classes.
     * this reduces the number of transitions and arcs and hence 
     * computational complexity.
     */
    rv = re_make_symbol_classes_from_pattern(nre, num_pattern, 
                                        res_flags, &pclsarr, &num_cl);
    if (rv) {
        rv = REGEX_ERROR_INVALID_CLASS;
        goto fail;
    }

    rv = make_nfa(nre, res_flags, num_pattern, &pnfa, pclsarr, num_cl);
    if (rv) {
        sal_free(pclsarr);
        rv = REGEX_ERROR_NO_NFA;
        goto fail;
    }

    if (make_dfa(num_pattern, &pdfa, &pnfa, pclsarr, num_cl) || !pdfa) {
        rv = REGEX_ERROR_NO_DFA;
        goto fail;
    }
    /* return the number of DFA states */
    rv = pdfa->n;

fail:
    for(p=0; p<num_pattern; p++) {
        if (nre[p]) {
            sal_free(nre[p]);
        }
    }
    sal_free(nre);
    if (pnfa) {
        nfa_free(pnfa);
    }
    if ((rv < 0) || (!ppdfa)) {
        if (pdfa) {
            dfa_free(pdfa);
        }
        return rv;
    }
    *ppdfa = (void*)pdfa;
    return rv;
}

typedef struct _re_state_compress_s {
    int state_id;
    unsigned int trmap[8];
    struct _re_state_compress_s *next;
} _re_state_compress;

static _re_state_compress* 
_re_find_compress_state(_re_state_compress **h, int state_id)
{
    /*
    _re_state_compress *ps;
    */
    
    while (*h && ((*h)->state_id != state_id)) {
        h = &(*h)->next;
    }
    return *h;
}

static int
_re_add_tr_to_compress_state(_re_state_compress **h,
                            int to_state_id, unsigned int *bmap)
{
    _re_state_compress *ps;
    int i;

    ps = _re_find_compress_state(h, to_state_id);
    if (!ps) {
        ps = sal_alloc(sizeof(_re_state_compress),"zipst");
        sal_memset(ps, 0, sizeof(_re_state_compress));
        /* add to list */
        ps->state_id = to_state_id;
        ps->next = *h;
        *h = ps;
    }

    for (i = 0; i < 8; i++) {
        ps->trmap[i] |= bmap[i];
    }
    return 0;
}

int bcm_regex_dfa_traverse(void *dfa, regex_dfa_state_cb compile_dfa_cb,
                            void *user_data)
{
    re_dfa_state_t *s;
    re_dfa_t  *dl = (re_dfa_t*) dfa;
    unsigned flags = 0;
    int c, to_state, rv = REGEX_ERROR_NONE;
    int match_idx, i, j, k, lc, has_trans;
    re_tr_cls  *cls;
    _re_state_compress *dlist[8], *pd;

    sal_memset(dlist, 0, sizeof(_re_state_compress*)*8);

    /*
     * Dummy callback to indicate start of iteration. Application might so
     * something specific like init or somethign.
     */
    if (compile_dfa_cb(DFA_TRAVERSE_START, -1, -1, -1, -1, -1, dl->n, user_data)) {
        return REGEX_ERROR;
    }

    /* call user provided callback for each DFA state. compress the
     * transitions so as to minimize the memory requirements */
    for (i = 0; i < dl->n; i++) {
        s = dl->s[i];
        flags = 0;
        match_idx = -1;
        has_trans = 0;
        for (j = 0; j < dl->num_cls; j++) {
            if (s->tlist[j] == -1) {
                continue;
            }
            has_trans++;
            c = j;
            to_state = s->tlist[j];
            cls = &dl->clsarr[c]; 

            _re_add_tr_to_compress_state(&dlist[to_state%8], 
                                         to_state, cls->trmap);
        }

        if (RE_DFA_IS_FINAL(s)) {
            match_idx = RE_DFA_FINAL(s) - 1;
            flags = DFA_STATE_FINAL;
            if (!has_trans) {
                if (compile_dfa_cb(DFA_STATE_FINAL, 
                                   match_idx, RE_DFA_STATEID(s), 
                                   -1, -1, -1, dl->n, user_data)) {
                    rv = REGEX_ERROR;
                    goto done;
                }
            }
        }

        for (j = 0; j < 8; j++) {
            while (dlist[j]) {
                pd = dlist[j];
                to_state = pd->state_id;
                lc = -1;
                for (k = 0; k < 256; k++) {
                    if (pd->trmap[k/32] & (1 << (k % 32))) {
                        if (lc == -1) {
                            lc = k;
                        }
                    } else if (lc >= 0) {
                        if (compile_dfa_cb(flags, match_idx, 
                                       RE_DFA_STATEID(s), lc, k-1, 
                                       pd->state_id, dl->n, 
                                       user_data)) {
                            rv = REGEX_ERROR;
                            goto done;
                        }
                        lc = -1;
                    }
                }
                if (lc >= 0)  {
                    if (compile_dfa_cb(flags, match_idx, RE_DFA_STATEID(s), 
                                   lc, 255, pd->state_id, dl->n, user_data)) {
                        rv = REGEX_ERROR;
                        goto done;
                    }
                }
                dlist[j] = pd->next;
                sal_free(pd);
            }
            dlist[j] = NULL;
        }
    }

    /* indicate end of traverse, so that any house keeping can be done
     * now */
    compile_dfa_cb(DFA_TRAVERSE_END, -1, -1, -1, -1, -1, dl->n, user_data);
done:
    for(i=0;i<8;i++) {
        while(dlist[i]) {
            pd = dlist[i]->next;
            sal_free(dlist[i]);
            dlist[i] = pd;
        }
    }
    return rv;
}

int bcm_regex_dfa_free(void *dfa)
{
    dfa_free((re_dfa_t*)dfa);
    return 0;
}

#ifndef BROADCOM_SDK 

int
main(int argc, char **argv)
{
    int rv, num_pat = 0, i, valid, bi;
    char *pc[64], tmp_re[512], c;
    unsigned int re_flags[512];
    unsigned int def_flags = 0 /* BCM_TR3_REGEX_CFLAG_EXPAND_LCUC */;
    void *dfa;
    FILE *fp;

    fp = fopen("patterns.txt", "r");

    bi = 0;
    while((c = getc(fp)) != EOF) {
        if ((c == '\n') || (c == '\r')) {
            valid = 0;
            i = 0;
            while (i < bi) {
                if ((tmp_re[i] != ' ') && (tmp_re[i] != '\t')) {
                    valid = 1;
                    break;
                }
            }
            
            if (valid) {
                tmp_re[bi] = '\0';
                pc[num_pat] = sal_alloc(512,"main");
                strcpy(pc[num_pat], tmp_re);
                re_flags[num_pat] = def_flags;
                num_pat++;
            }
            bi = 0;
            continue;
        }
        tmp_re[bi++] = c;
    }
    
    rv = bcm_regex_compile(pc, re_flags, num_pat, 0, &dfa);
    if (rv <= 0) {
        RE_SAL_DPRINT(("\n FAILED error=%d, re=%s\n", rv, argv[1]));
    }

    dfa_free(dfa);

    for (i=0; i<num_pat; i++) {
        sal_free(pc[i]);
    }

#if MEM_PROFILE == 1
    re_dump_mem_leak();
#endif

    return 0;
}
#endif


#else
int regex_supported = 0;
#endif
