/* 
   $Id: cint_c.tab.h,v 1.4 Broadcom SDK $
   $Copyright: Copyright 2012 Broadcom Corporation.
   This program is the proprietary software of Broadcom Corporation
   and/or its licensors, and may only be used, duplicated, modified
   or distributed pursuant to the terms and conditions of a separate,
   written license agreement executed between you and Broadcom
   (an "Authorized License").  Except as set forth in an Authorized
   License, Broadcom grants no license (express or implied), right
   to use, or waiver of any kind with respect to the Software, and
   Broadcom expressly reserves all rights in and to the Software
   and all intellectual property rights therein.  IF YOU HAVE
   NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
   IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
   ALL USE OF THE SOFTWARE.  
    
   Except as expressly set forth in the Authorized License,
    
   1.     This program, including its structure, sequence and organization,
   constitutes the valuable trade secrets of Broadcom, and you shall use
   all reasonable efforts to protect the confidentiality thereof,
   and to use this information only in connection with your use of
   Broadcom integrated circuit products.
    
   2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
   PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
   REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
   OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
   DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
   NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
   ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
   CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
   OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
   
   3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
   BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
   INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
   ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
   TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
   THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
   WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
   ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
*/

/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     XEOF = 0,
     IDENTIFIER = 258,
     CONSTANT = 259,
     STRING_LITERAL = 260,
     SIZEOF = 261,
     PTR_OP = 262,
     INC_OP = 263,
     DEC_OP = 264,
     LEFT_OP = 265,
     RIGHT_OP = 266,
     LE_OP = 267,
     GE_OP = 268,
     EQ_OP = 269,
     NE_OP = 270,
     AND_OP = 271,
     OR_OP = 272,
     MUL_ASSIGN = 273,
     DIV_ASSIGN = 274,
     MOD_ASSIGN = 275,
     ADD_ASSIGN = 276,
     SUB_ASSIGN = 277,
     LEFT_ASSIGN = 278,
     RIGHT_ASSIGN = 279,
     AND_ASSIGN = 280,
     XOR_ASSIGN = 281,
     OR_ASSIGN = 282,
     TYPE_NAME = 283,
     TYPEDEF = 284,
     EXTERN = 285,
     STATIC = 286,
     AUTO = 287,
     REGISTER = 288,
     CHAR = 289,
     SHORT = 290,
     INT = 291,
     LONG = 292,
     SIGNED = 293,
     UNSIGNED = 294,
     FLOAT = 295,
     DOUBLE = 296,
     CONST = 297,
     VOLATILE = 298,
     T_VOID = 299,
     STRUCT = 300,
     UNION = 301,
     ENUM = 302,
     ELLIPSIS = 303,
     CASE = 304,
     DEFAULT = 305,
     IF = 306,
     ELSE = 307,
     SWITCH = 308,
     WHILE = 309,
     DO = 310,
     FOR = 311,
     GOTO = 312,
     CONTINUE = 313,
     BREAK = 314,
     RETURN = 315,
     PRINT = 316,
     CINT = 317,
     ITERATOR = 318,
     MACRO = 319
   };
#endif
/* Tokens.  */
#define XEOF 0
#define IDENTIFIER 258
#define CONSTANT 259
#define STRING_LITERAL 260
#define SIZEOF 261
#define PTR_OP 262
#define INC_OP 263
#define DEC_OP 264
#define LEFT_OP 265
#define RIGHT_OP 266
#define LE_OP 267
#define GE_OP 268
#define EQ_OP 269
#define NE_OP 270
#define AND_OP 271
#define OR_OP 272
#define MUL_ASSIGN 273
#define DIV_ASSIGN 274
#define MOD_ASSIGN 275
#define ADD_ASSIGN 276
#define SUB_ASSIGN 277
#define LEFT_ASSIGN 278
#define RIGHT_ASSIGN 279
#define AND_ASSIGN 280
#define XOR_ASSIGN 281
#define OR_ASSIGN 282
#define TYPE_NAME 283
#define TYPEDEF 284
#define EXTERN 285
#define STATIC 286
#define AUTO 287
#define REGISTER 288
#define CHAR 289
#define SHORT 290
#define INT 291
#define LONG 292
#define SIGNED 293
#define UNSIGNED 294
#define FLOAT 295
#define DOUBLE 296
#define CONST 297
#define VOLATILE 298
#define T_VOID 299
#define STRUCT 300
#define UNION 301
#define ENUM 302
#define ELLIPSIS 303
#define CASE 304
#define DEFAULT 305
#define IF 306
#define ELSE 307
#define SWITCH 308
#define WHILE 309
#define DO 310
#define FOR 311
#define GOTO 312
#define CONTINUE 313
#define BREAK 314
#define RETURN 315
#define PRINT 316
#define CINT 317
#define ITERATOR 318
#define MACRO 319




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



#ifndef YYPUSH_DECLS
#  define YYPUSH_DECLS
struct cint_c_pstate;
typedef struct cint_c_pstate cint_c_pstate;
enum { YYPUSH_MORE = 4 };
#if defined __STDC__ || defined __cplusplus
int cint_c_parse (yyscan_t yyscanner, cint_cparser_t* cparser);
#else
int cint_c_parse ();
#endif
#if defined __STDC__ || defined __cplusplus
int cint_c_push_parse (cint_c_pstate *yyps, int yypushed_char, YYSTYPE const *yypushed_val, YYLTYPE const *yypushed_loc, yyscan_t yyscanner, cint_cparser_t* cparser);
#else
int cint_c_push_parse ();
#endif
#if defined __STDC__ || defined __cplusplus
int cint_c_pull_parse (cint_c_pstate *yyps, yyscan_t yyscanner, cint_cparser_t* cparser);
#else
int cint_c_pull_parse ();
#endif
#if defined __STDC__ || defined __cplusplus
cint_c_pstate * cint_c_pstate_new (void);
#else
cint_c_pstate * cint_c_pstate_new ();
#endif
#if defined __STDC__ || defined __cplusplus
void cint_c_pstate_delete (cint_c_pstate *yyps);
#else
void cint_c_pstate_delete ();
#endif
#endif

