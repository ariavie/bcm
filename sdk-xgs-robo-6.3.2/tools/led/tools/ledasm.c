/* $Id: ledasm.c 1.5 Broadcom SDK $
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
 * This is a simple assembler for the embedded processor used in the
 * SM-Lite LED controller.  The particulars of the instruction set
 * are documented elsewhere.
 *
 * As the programs for this processor will only be a few dozen bytes
 * long, this assembler is not feature rich.  It does not support
 * any type of modular assembly or linking.  It is assumed that the
 * user will just use their favorite preprocess of macros are desired.
 *
 * The assembler operates in two passes.  The first pass builds symbol
 * values, while the second pass actually does the assembly.
 *
 * Each line is of the form:
 *
 * <optlabel><tab><opcode><optargs><optcomment>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ledasm.h"


/* --------------------------- global vars -------------------------- */

int   g_errs;
int   g_warnings;
int   g_errors_enabled;		/* boolean flag */
int   g_warnings_enabled;	/* boolean flag */
int   g_curline;		    /* line number we're parsing */
int   g_curpc;			    /* PC of line being assembled */
int   g_pass;
uint8 g_program[256];


/*
 * --------------------------- symbol table routines --------------------------
 * this is designed to be foolproof and simple as possible, not fast.
 * therefore, it uses a simple linear search.  up to 256 labels are allowed.
 * only the first 32 characters of a label are retained.
 */

#define MAX_SYMBOLS 256
#define MAX_SYMLEN   32

/* symbol table itself
 *    + entry 0 is never used
 *    + we leave open the possibility that some entries get invalidated
 */
struct {
  int  valid;			/* 0=invalid entry, 1=valid entry */
  char label[MAX_SYMLEN];	/* label itself */
  long value;			/* value of label (32b) */
} symtab[MAX_SYMBOLS];

int num_labels;

/* --------------------------- helpers -------------------------- */

void
warning(char *msg)
{
    g_warnings++;

    if (g_warnings_enabled)
	printf("Warning, line %d: %s\n", g_curline, msg);
}


void
error(char *msg)
{
    g_errs++;

    if (g_errors_enabled)
	printf("Error, line %3d: %s\n", g_curline, msg);
}

void
init_tables(void)
{
    int i;

    num_labels = 0;

    for(i=0; i<MAX_SYMBOLS; i++) {
	symtab[i].valid    = 0;
	symtab[i].label[0] = 0;
	symtab[i].value    = 0;
    }
}


/*
 * look for a label that matches argument.
 * return 0 if not found, otherwise return the index.
 */
int
find_sym_idx(char *label)
{
    int i;
    int match = 0;

    for(i=0; i<=num_labels; i++) {
	if (symtab[i].valid && strcmp(label, symtab[i].label) == 0) {
	    match = i;
	    break;
	}
    }

    return match;
}


/*
 * add a label to the end of the symbol table.
 * we assume that the label isn't already in the table.
 * returns index to entry.
 */
int
add_symbol(char *label)
{
    num_labels++;
    if (num_labels >= MAX_SYMBOLS) {
	error("Error: symbol table overflow\n");
	return -1;
    }

    symtab[num_labels].valid = 1;
    strncpy(symtab[num_labels].label, label, MAX_SYMLEN);

    return num_labels;
}


void
dump_sym_table(FILE *fp)
{
    int i;

    fprintf(fp,"\n");
    fprintf(fp,"\n");
    fprintf(fp,"\n");
    fprintf(fp,"----- symbol table, %d symbols -----\n", num_labels);

    for(i=1; i<= num_labels; i++) {
	if (symtab[i].valid) {
	    fprintf(fp,"%12s\t0x%04lX\n", symtab[i].label, symtab[i].value);
	}
    }
}


/* this dumps the program ram to a file in a simple format:
 * there are no addresses, just 16 lines of 16 bytes per line,
 * printed in upper case hex.
 */
void
dump_hex_file(FILE *fp)
{
    int i;

    for(i=0; i<256; i++) {
	if (i < g_curpc)
	    fprintf(fp," %02X", g_program[i]);
	else
	    fprintf(fp," 00");
	if (i%16 == 15)
	    fprintf(fp,"\n");
    }
}

/*
 * this dumps the program ram to a c file in 
 * unsigned char array format
 */
void
dump_c_file(FILE *fp, char* basename)
{
    int i;
    
    /* normalize c identifiers */
    while(strchr(basename, '-')) {
	*strchr(basename, '-') = '_';
    }

    fprintf(fp, "/* %s.c -- autogenerated '%s' led program */\n\n", 
	    basename, basename);
    fprintf(fp, "const unsigned char ledproc_%s[] = {\n", 
	    basename);
    
    for(i=0; i<256; i++) {
	if (i < g_curpc)
	    fprintf(fp," 0x%02X,", g_program[i]);
	else
	    fprintf(fp," 0x00,");
	if (i%8 == 7)
	    fprintf(fp,"\n");
    }
    
    fprintf(fp, "};\n");
}


/* --------------------------- parser -------------------------- */

int parse_logop(char **p); /* forward declaration */

/* skip any blank chars */
void
skip_ws(char **p)
{
    while (isspace((unsigned)**p)) {
	(*p)++;
    }
}


/* syntax of a [comment]
 *    <null>
 *  | ;<anytext>
 */
void
parse_comment(char **p)
{
    char *pp = *p;

    skip_ws(&pp);

    if (*pp == ';') {
	while (*pp)
	    pp++;
    }

    *p = pp;
    return;
}


/* syntax of a [label]
 *    [_a-z][_a-z0-9]*
 * if we have a label, we return it in the provided buffer.
 * otherwise, we return a null string in the buffer.
 */
void
parse_label(char **p, char *buf)
{
    char *pp = *p;
    char *np = buf;
    char nowarn = 1;

    if (*pp == '_' || isalpha((unsigned)*pp)) {

	*np++ = *pp++;

	while (*pp == '_' || isalnum((unsigned)*pp)) {
	    if (np-buf < MAX_SYMLEN-1)
		*np++ = *pp;
	    else if (nowarn) {
		warning("excessively long label");
		nowarn = 0;
	    }
	    pp++;
	}

	*p = pp;	/* update pointer */
    }

    *np = '\0';	/* empty label */
}


/* -------------- expression evaluation ------------------ */

/* assumes lower case input */
int
ishexdigit(char c)
{
    return isdigit((unsigned)c) || (c >= 'a' && c <= 'f');
}

/* assumes lower case input */
int
isoctdigit(char c)
{
    return (c >= '0' && c <= '7');
}

int
hexval(char c)
{
    if (isdigit((unsigned)c))
	return c - '0';
    else
	return 10 + c - 'a';
}


/*
 * primop = ( conexpr )
 *        | integer
 *        | label
 */
int
parse_primop(char **p)
{
    char lbl[MAX_SYMLEN];
    char *pp;
    long val;
    int  idx;

    skip_ws(p);
    pp = *p;

    /* handle parenthesized expressions */
    if (*pp == '(') {
      pp++;	/* skip open paren */
	val = parse_logop(&pp);
	skip_ws(&pp);
	if (*pp != ')') {
	    error("missing closing paren");
	}
	*p = pp+1;	/* skip closing paren. */
	return val;
    }

    /* handle integers and labels */
    if (*pp == '0') {
	pp++;
	if (*pp == 'x') {
      /* hex number: 0x[0-9a-f]+ */
	    pp++;
	    if (!ishexdigit(*pp)) {
          /* must have at least one digit after the "0x" */
		error("badly formed hex number");
		*p = pp;
		return 0;
	    }
	    val = hexval(*pp);
	    pp++;
	    while (ishexdigit(*pp)) {
		val = 16*val + hexval(*pp);
		pp++;
	    }
	} else {
      /* octal number: 0[0-7]* */
	    val = 0;
	    while (isoctdigit(*pp)) {
		  val = 8*val + (*pp) - '0';
		  pp++;
	    }
	  }
    } else if (isdigit((unsigned)*pp)) {
    /* presumably decimal value */
	val = 0;
	while (isdigit((unsigned)*pp)) {
	    val = 10*val + (*pp) - '0';
	    pp++;
	}
    } else {
    /* assume it is a label */
	parse_label(p, lbl);
	if (strlen(lbl) == 0) {
      /* oops, what is it? */
	    error("error: expected constant expression");
	    return 0;
	}
	idx = find_sym_idx(lbl);
	if (idx) {
	    val = symtab[idx].value;
	} else {
      val = 0x00;	/* dummy value */
	    if (g_pass == 2)
		error("couldn't find symbol in second pass");
	}
	pp = *p; /* nullifies later assignment */
    }

    *p = pp;
    return val;
}


/*
 * unop = +conexpr
 *      | +conexpr
 *      | conexpr
 */
int
parse_unop(char **p)
{
    skip_ws(p);
    
    switch (**p) {

    case '+':
	(*p)++;	/* skip unary + */
	return parse_primop(p);

    case '-':
      (*p)++;	/* skip unary - */
	return -parse_primop(p);

    default:
	return parse_primop(p);
    }
}


/*
 * mulop = unop * unop
 *       | unop / unop
 *       | unop % unop
 *       | unop
 */
int
parse_mulop(char **p)
{
    int val1, val2;
    int done = 0;

    skip_ws(p);
    
    val1 = parse_unop(p);

    do {
	skip_ws(p);

	switch (**p) {

	case '*':
	    (*p)++;	/* skip * */
	    val2 = parse_unop(p);
	    val1 = val1 * val2;
	    break;

	case '/':
        (*p)++;	/* skip /  */
	    val2 = parse_unop(p);
	    if (val2 == 0) {
		if (g_pass == 2)
		    error("division by zero");
		done = 1;
		break;
	    }
	    val1 = val1 / val2;
	    break;

	case '%':
        (*p)++;	/* skip / */
	    val2 = parse_unop(p);
	    if (val2 == 0) {
		if (g_pass == 2)
		    error("modulus of zero");
		done = 1;
		break;
	    }
	    val1 = val1 % val2;
	    break;

	default:
	    done = 1;
	    break;
	}
    } while (!done);

    return val1;
}


/*
 * addop = mulop + mulop
 *       | mulop - mulop
 *       | mulop
 */
int
parse_addop(char **p)
{
    int val1, val2;
    int done = 0;

    skip_ws(p);
    
    val1 = parse_mulop(p);

    do {
	skip_ws(p);

	switch (**p) {

	case '+':
      (*p)++;	/* skip + */
	    val2 = parse_mulop(p);
	    val1 = val1+val2;
	    break;

	case '-':
      (*p)++;	/* skip - */
	    val2 = parse_mulop(p);
	    val1 = val1-val2;
	    break;

	default:
	    done = 1;
	    break;
	}

    } while (!done);

    return val1;
}


/*
 * shop = addop << addop
 *      | addop >> addop
 *      | addop
 */
int
parse_shop(char **p)
{
    int val1, val2;
    int done = 0;

    skip_ws(p);
    
    val1 = parse_addop(p);

    do {
	skip_ws(p);

	if (*(*p) == '>' && *(*p+1) == '>') {
      (*p) += 2;	/* skip >> */
	    val2 = parse_addop(p);
	    if (val2 < 0)
		error("negative shift count");
	    val1 = val1 >> val2;

	} else if (*(*p) == '<' && *(*p+1) == '<') {
      (*p) += 2;	/* skip >> */
	    val2 = parse_addop(p);
	    if (val2 < 0)
		error("negative shift count");
	    val1 = val1 << val2;

	} else
	    done = 1;

    } while (!done);

    return val1;
}


/*
 * logop = shop & shop
 *       | shop | shop
 *       | shop
 */
int
parse_logop(char **p)
{
    int val1, val2;
    int done = 0;

    skip_ws(p);
    
    val1 = parse_shop(p);

    do {
	skip_ws(p);

	switch (**p) {

	case '&':
        (*p)++;	/* skip &   */
	    val2 = parse_shop(p);
	    val1 = val1 & val2;
	    break;

	case '|':
        (*p)++;	/* skip | */
	    val2 = parse_shop(p);
	    val1 = val1 | val2;
	    break;

	default:
	    done = 1;
	    break;
	}
    } while (!done);

    return val1;
}


/* parse an argument -- is must evaluate to a constant value */
int
parse_conexpr(char **p)
{
    int val = parse_logop(p);
    return val;
}


/* -------------- argument parsing ------------------ */

/*
 * arg must be one of A, B, #.
 * we return the encoded field and optional 2nd byte.
 */
void
parse_ss(char **p, int *field, int *twobyte, int *byte2)
{
    char lbl[MAX_SYMLEN];
    char *pp;

    skip_ws(p);
    pp = *p;

    parse_label(p, lbl);

    if (strcmp(lbl,"a") == 0) {
	*twobyte = 0;
	*field   = 0;	/* 00 = A */
	return;
    }

    if (strcmp(lbl,"b") == 0) {
	*twobyte = 0;
	*field   = 1;	/* 01 = B */
	return;
    }

    *p = pp;		/* put back to before label parse attempt */
    *byte2 = parse_conexpr(p);
    *twobyte = 1;
    *twobyte = 1;
    *field   = 2;	/* 10 = # */
    return;
}


/*
 * arg must be one of A, B, #, (A), (B), (#), or CY
 * we return the encoded field and optional 2nd byte.
 */
void
parse_sssx(char **p, int *field, int *twobyte, int *byte2)
{
    char lbl[MAX_SYMLEN];
    char *pp;

    skip_ws(p);
    pp = *p;

    if (*pp == '(') {
      /* must be indirect: one of (A), (B), or (#) */
      pp++; /* skip opening paren */
	parse_ss(&pp, field, twobyte, byte2);
	if (*pp != ')') {
	    error("missing closing paren");
	} else {
	    pp++;
	    *field |= 0x4;	/* turn A,B,# into (A),(B),(#) */
	}
	*p = pp;
	return;
    }

    /* maybe it is the odd case of CY (used only by push CY) */
    parse_label(p, lbl);
    if (strcmp(lbl,"cy") == 0) {
      *field = 7;	/* magic CY symbol */
	  *twobyte = 0;
	  return;
    }

    /* just one of A, B, or #.  */
    parse_ss(&pp, field, twobyte, byte2);
    *p = pp;
}


/*
 * arg must be one of A, B, #, (A), (B), (#)
 * we return the encoded field and optional 2nd byte.
 */
void
parse_sss(char **p, int *field, int *twobyte, int *byte2)
{
    parse_sssx(p, field, twobyte, byte2);
    if (*field == 7)
	error("CY is not a valid argument for this opcode");
}


/*
 * arg must be one of A, B, (A), (B), (#)
 * we return the encoded field and optional 2nd byte.
 */
void
parse_ddd(char **p, int *field, int *twobyte, int *byte2)
{
    parse_sss(p, field, twobyte, byte2);
    if (*field == 2)
	error("immediate destination");
}


/* arg must be one of A or B. */
void
parse_d(char **p, int *field)
{
    char lbl[MAX_SYMLEN];
    char *pp;

    skip_ws(p);
    pp = *p;

    parse_label(p, lbl);

    if (strcmp(lbl,"a") == 0) {
      *field = 0;	/* 00 = A */
    } else if (strcmp(lbl,"b") == 0) {
      *field = 1;	/* 01 = B */
    } else {
	error("expecting either 'A' or 'B' as first arg");
	*field = 0;
    }
}


/* emit byte to output */
void
emit(int v)
{
    static int size = 0;
    if (g_curpc > 0xFF) {
	if (size == 0)
	    error("program is too long");
	size = 1;
	return;
    }

    if (v >= -128 && v <= 255) {
	g_program[g_curpc] = (v & 0xFF);
	g_curpc++;
    } else {
	error("value out of bounds -128 <= val <= 255");
    }
}


/* parse arguments; number and type depend on opcode */
void
parse_args(char **p, int op, char *label)
{
    int idx;
    int field1,field2;
    int val, val2;
    int twobyte, twobyte2;

    switch (optable[op].token) {

    case PO_EQU:
      /* get value */
	val = parse_conexpr(p);
	/* update symbol table */
	if (strlen(label) == 0) {
	    error("EQU must have a label");
	    return;
	}
	if (g_pass == 1) {
	    idx = find_sym_idx(label);
	    if (idx != 0) {
		error("redefinition of EQU label");
		return;
	    }
	    idx = add_symbol(label);
	    if (idx < 0)
		return;
	    symtab[num_labels].value = val;
	}
	break;

    case PO_SET:
      /* get value */
	val = parse_conexpr(p);
	/* update symbol table */
	if (strlen(label) == 0) {
	    error("SET must have a label");
	    return;
	}
	idx = find_sym_idx(label);
	if (idx == 0) {
	    idx = add_symbol(label);
	    if (idx < 0)
		return;
	}
	symtab[num_labels].value = val;
	break;

    case OP_CLC:
    case OP_STC:
    case OP_CMC:
    case OP_RET:
    case OP_TAND:
    case OP_TOR:
    case OP_TXOR:
    case OP_TINV:
    case OP_PACK:
    case OP_POP:
	emit(optable[op].opcode);
	break;

    case OP_CALL:
    case OP_JMP:
	emit(optable[op].opcode);
	val = parse_conexpr(p);
	emit(val);
	break;

    case OP_PUSH:
	parse_sssx(p, &field1, &twobyte, &val);
	emit(optable[op].opcode | field1);
	if (twobyte)
	    emit(val);
	break;

    case OP_PORT:
    case OP_SEND:
	parse_sss(p, &field1, &twobyte, &val);
	emit(optable[op].opcode | field1);
	if (twobyte)
	    emit(val);
	break;

    case OP_PUSHST:
	parse_ss(p, &field1, &twobyte, &val);
	emit(optable[op].opcode | field1);
	if (twobyte)
	    emit(val);
	break;

    case OP_JZ:
    case OP_JC:
    case OP_JT:
    case OP_JNZ:
    case OP_JNC:
    case OP_JNT:
	emit(optable[op].opcode);
	val = parse_conexpr(p);
	emit(val);
	break;

    case OP_INC:
    case OP_DEC:
    case OP_ROL:
    case OP_ROR:
	parse_ddd(p, &field1, &twobyte, &val);
	emit(optable[op].opcode | field1);
	if (twobyte)
	    emit(val);
	break;

    case OP_ADD:
    case OP_SUB:
    case OP_CMP:
    case OP_AND:
    case OP_OR:
    case OP_XOR:
	parse_d(p, &field1);
	skip_ws(p);
	if (**p != ',') {
	    error("this op needs two args");
	    return;
	}
	(*p)++;	/* skip comma */
	parse_sss(p, &field2, &twobyte, &val);
	emit(optable[op].opcode | (field1 << 3) | field2);
	if (twobyte)
	    emit(val);
	break;

    case OP_TST:
    case OP_BIT:
    case OP_LD:
	parse_ddd(p, &field1, &twobyte, &val);
	skip_ws(p);
	if (**p != ',') {
	    error("this op needs two args");
	    return;
	}
	(*p)++;	/* skip comma */
	if (optable[op].token == OP_LD)
	    parse_sss(p, &field2, &twobyte2, &val2);
	else
	    parse_ss(p, &field2, &twobyte2, &val2);
	if (twobyte && twobyte2) {
	    error("<op> #,(#) format is not allowed (3 bytes)");
	}
	emit(optable[op].opcode | (field1 << 4) | field2);
	if (twobyte)
	    emit(val);
	if (twobyte2)
	    emit(val2);
	break;

    default:
	error("internal error: impossible case");
	break;
    }
}


/*
 * syntax of a [mainpart]
 *     <null>
 *   | ^<label>:
 *   | ^<optlabel><ws><opcode><ws><args>
 */
void
parse_mainpart(char **p)
{
  char labelbuf[MAX_SYMLEN];	/* storage for line label */
  char opbuf[MAX_SYMLEN];	/* storage for opcode */
    int  op, idx;
    int  colon = 0;

    /* parse for optional label */
    parse_label(p, labelbuf);
    if (strlen(labelbuf)) {
      /* colon after label is manditory */
	colon = (**p == ':');
	if (colon)
	    (*p)++;
    }
    skip_ws(p);

    /* parse for opcode */
    op = 0;
    parse_label(p, opbuf);
    if (opbuf[0]) {	/* non-empty */
	while (optable[op].token != OP_SENTINAL) {
	    if (strcmp(optable[op].label,opbuf) == 0)
		break;
	    op++;
	}
	if (optable[op].token == OP_SENTINAL) {
	    error("bad opcode");
	    return;
	}
    }

    /* 
     * this minimizes chance of badly placed opcode
     * being mistaken for a label
     */
    if (labelbuf[0] && !opbuf[0] && !colon) {
	error("standalone labels must be followed by a colon");
	return;
    }

    /* add label to symbol table if it isn't an EQU-type */
    if (g_pass == 1 && strlen(labelbuf) > 0) {
	if (!opbuf[0] ||
	    (optable[op].token != PO_SET && optable[op].token != PO_EQU)) {
	    idx = find_sym_idx(labelbuf);
	    if (idx != 0) {
		error("label already defined");
	    } else {
		idx = add_symbol(labelbuf);
		if (idx < 0)
		    return;
		symtab[idx].value = g_curpc;
	    }
	}
    }

    /* parse for args, specific to each opcode */
    if (opbuf[0])
	parse_args(p, op, labelbuf);
}


/* 
 * syntax of a line: 
 *  [mainpart]?[comment]?
 */
void
parse_line(char *line)
{
    char *p = line;
    parse_mainpart(&p);
    parse_comment(&p);
    if (*p) {
	warning("extra garbage at end of line");
    }
}


void
lower_buf(char *dst, char *src)
{
    while (*src) {
	*dst = tolower(*src);
	dst++;
	src++;
    }
    *dst = '\0';
}


/* open the file after appending the extension. */
FILE *
my_fopen(char *basename, char *ext, char *access)
{
    char name[256];
    FILE *fp;

    strcpy(name,basename);
    strcat(name,ext);
    fp = fopen(name, access);
    if (!fp) {
	printf("Error opening file '%s'\n", name);
    }
    return fp;
}

void
my_unlink(char *basename, char *ext)
{
    char name[256];

    strcpy(name,basename);
    strcat(name,ext);

    (void) remove(name);
}

int
parse_file(char *basename)
{
    char rawbuf[1024];
    char linebuf[1024];
    FILE *in_fp  = 0;
    FILE *lst_fp = 0;
    FILE *hex_fp = 0;
    FILE *c_fp   = 0;

    in_fp = my_fopen(basename, ".asm", "r");
    if (in_fp == NULL) {
	printf("Error: file %s.asm not found\n", basename);
	return -1;
    }

    if (g_pass == 1) {
	g_errs = g_warnings = 0;
	init_tables();
    } else {
	lst_fp = my_fopen(basename, ".lst", "w");
	if (lst_fp == NULL) {
	    printf("Error: could not open output file %s.lst\n", basename);
	    return -1;
	}
	hex_fp = my_fopen(basename, ".hex", "w");
	if (hex_fp == NULL) {
	    printf("Error: could not open output file %s.hex\n", basename);
	    return -1;
	}
	c_fp = my_fopen(basename, ".c", "w");
	if (c_fp == NULL) {
	    printf("Error: could not open output file %s.c\n", basename);
	    return -1;
	}
    }

    printf("starting pass %d ...\n",g_pass);
    g_curline = 0;
    g_curpc = 0;

    while (fgets(rawbuf, sizeof(rawbuf), in_fp)) {
	int start_pc = g_curpc;
	g_curline++;
	lower_buf(linebuf,rawbuf);
	parse_line(linebuf);
	if (g_pass == 2) {
      /* format listing */
	    fprintf(lst_fp, "%02X: ", start_pc);
	    if ((g_curpc - start_pc) < 1) fprintf(lst_fp, "   ");
	    else fprintf(lst_fp, "%02X ", g_program[start_pc]);
	    if ((g_curpc - start_pc) < 2) fprintf(lst_fp, "   ");
	    else fprintf(lst_fp, "%02X ", g_program[start_pc+1]);
	    fprintf(lst_fp, "  %s", rawbuf);
	}
    }

    if (g_pass == 2) {
	dump_sym_table(lst_fp);
	dump_hex_file(hex_fp);
	dump_c_file(c_fp, basename);
	fclose(lst_fp);
	fclose(hex_fp);
	fclose(c_fp);
    }

    fclose(in_fp);

    return 0;
}


/* --------------------------- main program -------------------------- */
void
help(void)
{
    printf("Usage: ledasm <filename>\n");
    printf("   This scans <filename>.asm, and produces <filename>.lst and <filename>.hex\n");
    exit(1);
}


int
main(int argc, char *argv[])
{
    if (argc != 2)
	help();

    if (strlen(argv[1]) > 250) {
	printf("How about a shorter source file name?\n");
	exit(1);
    }

    /* at the current time, these can't be changed from the command line */
    g_errors_enabled = 1;
    g_warnings_enabled = 1;

    /* ---------------- first pass ------------------ */

    g_pass = 1;
    if (parse_file(argv[1]) < 0)
	g_errs++;
    else {
      /* ---------------- second pass ------------------ */

	g_pass = 2;
	if (parse_file(argv[1]) < 0)
	    g_errs++;
    }

    printf("%8d errors, %d warnings\n", g_errs, g_warnings);

    if (g_errs) {
	my_unlink(argv[1],".lst");
	my_unlink(argv[1],".hex");
	exit(1);
    }

    exit(0);
}



