/* 
   $Id: 36cbe0cd32204a4f395b3916bfb00980ede711bf $
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

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 1

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 1

/* Substitute the variable and function names.  */
#define yyparse         cint_c_parse
#define yypush_parse    cint_c_push_parse
#define yypull_parse    cint_c_pull_parse
#define yypstate_new    cint_c_pstate_new
#define yypstate_delete cint_c_pstate_delete
#define yypstate        cint_c_pstate
#define yylex           cint_c_lex
#define yyerror         cint_c_error
#define yylval          cint_c_lval
#define yychar          cint_c_char
#define yydebug         cint_c_debug
#define yynerrs         cint_c_nerrs
#define yylloc          cint_c_lloc

/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "cint_grammar.y"

/*
 * $Id: cint_grammar.y,v 1.35 2012/11/08 19:50:53 dkelley Exp $
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
 *
 * File:        cint_grammar.y
 * Purpose:     CINT C Parser
 *
 */

/* Line 189 of yacc.c  */
#line 56 "cint_grammar.y"


#ifndef LONGEST_SOURCE_LINE
#define LONGEST_SOURCE_LINE 256
#endif

 typedef struct cint_c_parser_s {
     int x; 
 } cint_c_parser_t; 

typedef void* yyscan_t;

#define YY_TYPEDEF_YY_SCANNER_T
#define YYERROR_VERBOSE 1

#include "cint_config.h"
#include "cint_parser.h"

#include "cint_yy.h"
#include "cint_c.tab.h"


void cint_c_error(YYLTYPE * locp, yyscan_t yyscanner, cint_cparser_t * cp,
                  const char *msg);
extern int cint_c_lex(YYSTYPE * yylval_param, YYLTYPE * yylloc_param,
                      yyscan_t yyscanner);
char *cint_current_line(yyscan_t yyscanner, char *const lineBuffer, const int lineLen,
                        int *column, int *tokLen, char **curFile, int *curLine);


#if CINT_CONFIG_INCLUDE_PARSER == 1


#include "cint_interpreter.h"




/* Line 189 of yacc.c  */
#line 138 "cint_c.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


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
struct yypstate;
typedef struct yypstate yypstate;
enum { YYPUSH_MORE = 4 };

#if defined __STDC__ || defined __cplusplus
int yyparse (yyscan_t yyscanner, cint_cparser_t* cparser);
#else
int yyparse ();
#endif
#if defined __STDC__ || defined __cplusplus
int yypush_parse (yypstate *yyps, int yypushed_char, YYSTYPE const *yypushed_val, YYLTYPE const *yypushed_loc, yyscan_t yyscanner, cint_cparser_t* cparser);
#else
int yypush_parse ();
#endif
#if defined __STDC__ || defined __cplusplus
int yypull_parse (yypstate *yyps, yyscan_t yyscanner, cint_cparser_t* cparser);
#else
int yypull_parse ();
#endif
#if defined __STDC__ || defined __cplusplus
yypstate * yypstate_new (void);
#else
yypstate * yypstate_new ();
#endif
#if defined __STDC__ || defined __cplusplus
void yypstate_delete (yypstate *yyps);
#else
void yypstate_delete ();
#endif
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 356 "cint_c.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  220
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1458

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  89
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  69
/* YYNRULES -- Number of rules.  */
#define YYNRULES  255
/* YYNRULES -- Number of states.  */
#define YYNSTATES  401

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   319

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    76,     2,     2,     2,    78,    71,     2,
      65,    66,    72,    73,    70,    74,    69,    77,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    84,    86,
      79,    85,    80,    83,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    67,     2,    68,    81,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    87,    82,    88,    75,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    13,    15,    20,    24,
      29,    33,    37,    40,    43,    45,    49,    51,    54,    57,
      60,    63,    68,    70,    72,    74,    76,    78,    80,    82,
      88,    93,    95,    99,   103,   107,   109,   113,   117,   119,
     123,   127,   129,   133,   137,   141,   145,   147,   151,   155,
     157,   161,   163,   167,   169,   173,   175,   179,   181,   185,
     187,   193,   195,   199,   201,   203,   205,   207,   209,   211,
     213,   215,   217,   219,   221,   223,   227,   229,   232,   236,
     238,   241,   243,   246,   248,   251,   253,   257,   259,   263,
     265,   267,   269,   271,   273,   275,   277,   279,   281,   283,
     285,   287,   289,   291,   293,   295,   297,   303,   308,   311,
     313,   315,   317,   320,   324,   326,   330,   332,   337,   343,
     346,   348,   352,   354,   358,   361,   363,   366,   368,   370,
     372,   375,   377,   379,   383,   388,   392,   397,   401,   403,
     406,   409,   413,   415,   418,   420,   422,   426,   429,   432,
     434,   436,   439,   441,   443,   446,   450,   453,   457,   461,
     466,   469,   473,   477,   482,   484,   488,   493,   495,   499,
     501,   503,   505,   507,   509,   511,   513,   515,   517,   520,
     524,   526,   528,   530,   532,   534,   536,   538,   540,   542,
     544,   546,   548,   550,   552,   554,   556,   558,   560,   562,
     564,   566,   568,   570,   572,   574,   576,   578,   580,   582,
     584,   586,   588,   590,   592,   594,   596,   598,   600,   602,
     604,   607,   611,   615,   620,   624,   626,   629,   633,   635,
     638,   640,   643,   645,   648,   654,   662,   668,   674,   680,
     688,   695,   703,   709,   714,   718,   721,   724,   727,   731,
     733,   736,   738,   740,   742,   747
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     155,     0,    -1,     3,    -1,     4,    -1,     5,    -1,    65,
     109,    66,    -1,    90,    -1,    91,    67,   109,    68,    -1,
      91,    65,    66,    -1,    91,    65,    92,    66,    -1,    91,
      69,     3,    -1,    91,     7,     3,    -1,    91,     8,    -1,
      91,     9,    -1,   107,    -1,    92,    70,   107,    -1,    91,
      -1,     8,    93,    -1,     9,    93,    -1,    94,    95,    -1,
       6,    93,    -1,     6,    65,   135,    66,    -1,    71,    -1,
      72,    -1,    73,    -1,    74,    -1,    75,    -1,    76,    -1,
      93,    -1,    65,    44,    72,    66,    95,    -1,    65,    32,
      66,    95,    -1,    95,    -1,    96,    72,    95,    -1,    96,
      77,    95,    -1,    96,    78,    95,    -1,    96,    -1,    97,
      73,    96,    -1,    97,    74,    96,    -1,    97,    -1,    98,
      10,    97,    -1,    98,    11,    97,    -1,    98,    -1,    99,
      79,    98,    -1,    99,    80,    98,    -1,    99,    12,    98,
      -1,    99,    13,    98,    -1,    99,    -1,   100,    14,    99,
      -1,   100,    15,    99,    -1,   100,    -1,   101,    71,   100,
      -1,   101,    -1,   102,    81,   101,    -1,   102,    -1,   103,
      82,   102,    -1,   103,    -1,   104,    16,   103,    -1,   104,
      -1,   105,    17,   104,    -1,   105,    -1,   105,    83,   109,
      84,   106,    -1,   106,    -1,    93,   108,   107,    -1,    85,
      -1,    18,    -1,    19,    -1,    20,    -1,    21,    -1,    22,
      -1,    23,    -1,    24,    -1,    25,    -1,    26,    -1,    27,
      -1,   107,    -1,   109,    70,   107,    -1,   106,    -1,   112,
      86,    -1,   112,   113,    86,    -1,   115,    -1,   115,   112,
      -1,   116,    -1,   116,   112,    -1,   127,    -1,   127,   112,
      -1,   114,    -1,   113,    70,   114,    -1,   128,    -1,   128,
      85,   138,    -1,    30,    -1,    29,    -1,    31,    -1,    32,
      -1,    33,    -1,    44,    -1,    34,    -1,    35,    -1,    36,
      -1,    37,    -1,    40,    -1,    41,    -1,    38,    -1,    39,
      -1,   117,    -1,   123,    -1,    28,    -1,   118,     3,    87,
     119,    88,    -1,   118,    87,   119,    88,    -1,   118,     3,
      -1,    45,    -1,    46,    -1,   120,    -1,   119,   120,    -1,
     126,   121,    86,    -1,   122,    -1,   121,    70,   122,    -1,
     128,    -1,    47,    87,   124,    88,    -1,    47,     3,    87,
     124,    88,    -1,    47,     3,    -1,   125,    -1,   124,    70,
     125,    -1,     3,    -1,     3,    85,   110,    -1,   116,   126,
      -1,   116,    -1,   127,   126,    -1,   127,    -1,    42,    -1,
      43,    -1,   130,   129,    -1,   129,    -1,     3,    -1,    65,
     128,    66,    -1,   129,    67,   110,    68,    -1,   129,    67,
      68,    -1,   129,    65,   132,    66,    -1,   129,    65,    66,
      -1,    72,    -1,    72,   131,    -1,    72,   130,    -1,    72,
     131,   130,    -1,   127,    -1,   131,   127,    -1,   133,    -1,
     134,    -1,   133,    70,   134,    -1,   112,   128,    -1,   112,
     136,    -1,   112,    -1,   126,    -1,   126,   136,    -1,   130,
      -1,   137,    -1,   130,   137,    -1,    65,   136,    66,    -1,
      67,    68,    -1,    67,   110,    68,    -1,   137,    67,    68,
      -1,   137,    67,   110,    68,    -1,    65,    66,    -1,    65,
     132,    66,    -1,   137,    65,    66,    -1,   137,    65,   132,
      66,    -1,   107,    -1,    87,   139,    88,    -1,    87,   139,
      70,    88,    -1,   138,    -1,   139,    70,   138,    -1,   146,
      -1,   147,    -1,   148,    -1,   151,    -1,   152,    -1,   153,
      -1,   154,    -1,   141,    -1,   145,    -1,    61,   151,    -1,
      61,    28,    86,    -1,     6,    -1,    29,    -1,    30,    -1,
      31,    -1,    32,    -1,    33,    -1,    34,    -1,    35,    -1,
      36,    -1,    37,    -1,    38,    -1,    39,    -1,    40,    -1,
      41,    -1,    42,    -1,    43,    -1,    44,    -1,    45,    -1,
      46,    -1,    47,    -1,    49,    -1,    50,    -1,    51,    -1,
      52,    -1,    53,    -1,    54,    -1,    55,    -1,    56,    -1,
      57,    -1,    58,    -1,    59,    -1,    60,    -1,    62,    -1,
      61,    -1,     3,    -1,    28,    -1,     5,    -1,     4,    -1,
     142,    -1,   143,    -1,   144,   143,    -1,    62,   144,    86,
      -1,     3,    84,   140,    -1,    49,   110,    84,   140,    -1,
      50,    84,   140,    -1,   111,    -1,    87,    88,    -1,    87,
     150,    88,    -1,   111,    -1,   149,   111,    -1,   140,    -1,
     150,   140,    -1,    86,    -1,   109,    86,    -1,    51,    65,
     109,    66,   140,    -1,    51,    65,   109,    66,   140,    52,
     140,    -1,    53,    65,   109,    66,   140,    -1,    64,    65,
      92,    66,    86,    -1,    54,    65,   109,    66,   140,    -1,
      55,   140,    54,    65,   109,    66,    86,    -1,    56,    65,
     151,   151,    66,   140,    -1,    56,    65,   151,   151,   109,
      66,   140,    -1,    63,    65,    92,    66,   140,    -1,    63,
      65,    66,   140,    -1,    57,     3,    86,    -1,    58,    86,
      -1,    59,    86,    -1,    60,    86,    -1,    60,   109,    86,
      -1,   156,    -1,   155,   156,    -1,     0,    -1,   157,    -1,
     140,    -1,   112,   128,   149,   148,    -1,   112,   128,   148,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   101,   101,   102,   103,   104,   108,   109,   110,   114,
     118,   122,   126,   127,   131,   132,   140,   141,   142,   143,
     144,   145,   149,   150,   151,   152,   153,   154,   158,   161,
     163,   167,   168,   169,   170,   174,   175,   176,   180,   181,
     182,   186,   187,   188,   189,   190,   194,   195,   196,   200,
     201,   205,   206,   210,   211,   215,   216,   220,   221,   225,
     226,   231,   232,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   264,   265,   272,   276,   277,   284,
     285,   286,   287,   288,   289,   293,   294,   298,   299,   304,
     305,   306,   307,   308,   312,   313,   314,   315,   316,   317,
     318,   319,   320,   321,   322,   323,   327,   328,   329,   333,
     334,   338,   339,   343,   351,   352,   356,   362,   363,   364,
     368,   369,   373,   374,   378,   379,   380,   381,   385,   386,
     390,   394,   398,   399,   400,   402,   404,   409,   416,   417,
     418,   419,   423,   424,   429,   434,   435,   443,   447,   448,
     463,   464,   468,   469,   470,   474,   475,   476,   477,   478,
     479,   480,   481,   482,   486,   487,   488,   492,   493,   497,
     498,   499,   504,   505,   506,   507,   508,   509,   514,   515,
     519,   520,   521,   522,   523,   524,   525,   526,   527,   528,
     529,   530,   531,   532,   533,   534,   535,   536,   537,   538,
     539,   540,   541,   542,   543,   544,   545,   546,   547,   548,
     549,   550,   551,   552,   558,   559,   560,   561,   562,   566,
     567,   575,   578,   579,   580,   584,   587,   588,   594,   595,
     599,   600,   608,   609,   613,   617,   621,   622,   626,   630,
     634,   638,   642,   646,   653,   654,   655,   656,   657,   662,
     688,   714,   723,   725,   729,   730
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "XEOF", "error", "$undefined", "IDENTIFIER", "CONSTANT",
  "STRING_LITERAL", "SIZEOF", "PTR_OP", "INC_OP", "DEC_OP", "LEFT_OP",
  "RIGHT_OP", "LE_OP", "GE_OP", "EQ_OP", "NE_OP", "AND_OP", "OR_OP",
  "MUL_ASSIGN", "DIV_ASSIGN", "MOD_ASSIGN", "ADD_ASSIGN", "SUB_ASSIGN",
  "LEFT_ASSIGN", "RIGHT_ASSIGN", "AND_ASSIGN", "XOR_ASSIGN", "OR_ASSIGN",
  "TYPE_NAME", "TYPEDEF", "EXTERN", "STATIC", "AUTO", "REGISTER", "CHAR",
  "SHORT", "INT", "LONG", "SIGNED", "UNSIGNED", "FLOAT", "DOUBLE", "CONST",
  "VOLATILE", "T_VOID", "STRUCT", "UNION", "ENUM", "ELLIPSIS", "CASE",
  "DEFAULT", "IF", "ELSE", "SWITCH", "WHILE", "DO", "FOR", "GOTO",
  "CONTINUE", "BREAK", "RETURN", "PRINT", "CINT", "ITERATOR", "MACRO",
  "'('", "')'", "'['", "']'", "'.'", "','", "'&'", "'*'", "'+'", "'-'",
  "'~'", "'!'", "'/'", "'%'", "'<'", "'>'", "'^'", "'|'", "'?'", "':'",
  "'='", "';'", "'{'", "'}'", "$accept", "primary_expression",
  "postfix_expression", "argument_expression_list", "unary_expression",
  "unary_operator", "cast_expression", "multiplicative_expression",
  "additive_expression", "shift_expression", "relational_expression",
  "equality_expression", "and_expression", "exclusive_or_expression",
  "inclusive_or_expression", "logical_and_expression",
  "logical_or_expression", "conditional_expression",
  "assignment_expression", "assignment_operator", "expression",
  "constant_expression", "declaration", "declaration_specifiers",
  "init_declarator_list", "init_declarator", "storage_class_specifier",
  "type_specifier", "struct_or_union_specifier", "struct_or_union",
  "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "enum_specifier",
  "enumerator_list", "enumerator", "specifier_qualifier_list",
  "type_qualifier", "declarator", "direct_declarator", "pointer",
  "type_qualifier_list", "parameter_type_list", "parameter_list",
  "parameter_declaration", "type_name", "abstract_declarator",
  "direct_abstract_declarator", "initializer", "initializer_list",
  "statement", "print_statement", "keyword_arg", "cint_argument",
  "cint_argument_list", "cint_statement", "labeled_statement",
  "declaration_statement", "compound_statement", "declaration_list",
  "statement_list", "expression_statement", "selection_statement",
  "iteration_statement", "jump_statement", "translation_unit",
  "external_declaration", "function_definition", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,    40,    41,    91,    93,    46,
      44,    38,    42,    43,    45,   126,    33,    47,    37,    60,
      62,    94,   124,    63,    58,    61,    59,   123,   125
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    89,    90,    90,    90,    90,    91,    91,    91,    91,
      91,    91,    91,    91,    92,    92,    93,    93,    93,    93,
      93,    93,    94,    94,    94,    94,    94,    94,    95,    95,
      95,    96,    96,    96,    96,    97,    97,    97,    98,    98,
      98,    99,    99,    99,    99,    99,   100,   100,   100,   101,
     101,   102,   102,   103,   103,   104,   104,   105,   105,   106,
     106,   107,   107,   108,   108,   108,   108,   108,   108,   108,
     108,   108,   108,   108,   109,   109,   110,   111,   111,   112,
     112,   112,   112,   112,   112,   113,   113,   114,   114,   115,
     115,   115,   115,   115,   116,   116,   116,   116,   116,   116,
     116,   116,   116,   116,   116,   116,   117,   117,   117,   118,
     118,   119,   119,   120,   121,   121,   122,   123,   123,   123,
     124,   124,   125,   125,   126,   126,   126,   126,   127,   127,
     128,   128,   129,   129,   129,   129,   129,   129,   130,   130,
     130,   130,   131,   131,   132,   133,   133,   134,   134,   134,
     135,   135,   136,   136,   136,   137,   137,   137,   137,   137,
     137,   137,   137,   137,   138,   138,   138,   139,   139,   140,
     140,   140,   140,   140,   140,   140,   140,   140,   141,   141,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   142,   142,   142,   142,   142,   142,
     142,   142,   142,   142,   143,   143,   143,   143,   143,   144,
     144,   145,   146,   146,   146,   147,   148,   148,   149,   149,
     150,   150,   151,   151,   152,   152,   152,   152,   153,   153,
     153,   153,   153,   153,   154,   154,   154,   154,   154,   155,
     155,   155,   156,   156,   157,   157
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     3,     1,     4,     3,     4,
       3,     3,     2,     2,     1,     3,     1,     2,     2,     2,
       2,     4,     1,     1,     1,     1,     1,     1,     1,     5,
       4,     1,     3,     3,     3,     1,     3,     3,     1,     3,
       3,     1,     3,     3,     3,     3,     1,     3,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       5,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     1,     2,     3,     1,
       2,     1,     2,     1,     2,     1,     3,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     5,     4,     2,     1,
       1,     1,     2,     3,     1,     3,     1,     4,     5,     2,
       1,     3,     1,     3,     2,     1,     2,     1,     1,     1,
       2,     1,     1,     3,     4,     3,     4,     3,     1,     2,
       2,     3,     1,     2,     1,     1,     3,     2,     2,     1,
       1,     2,     1,     1,     2,     3,     2,     3,     3,     4,
       2,     3,     3,     4,     1,     3,     4,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     3,     3,     4,     3,     1,     2,     3,     1,     2,
       1,     2,     1,     2,     5,     7,     5,     5,     5,     7,
       6,     7,     5,     4,     3,     2,     2,     2,     3,     1,
       2,     1,     1,     1,     4,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,   251,     2,     3,     4,     0,     0,     0,   105,    90,
      89,    91,    92,    93,    95,    96,    97,    98,   101,   102,
      99,   100,   128,   129,    94,   109,   110,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    22,    23,    24,    25,    26,    27,
     232,     0,     6,    16,    28,     0,    31,    35,    38,    41,
      46,    49,    51,    53,    55,    57,    59,    61,    74,     0,
     225,     0,    79,    81,   103,     0,   104,    83,   253,   176,
     177,   169,   170,   171,   172,   173,   174,   175,     0,   249,
     252,     0,     2,     0,    20,     0,    17,    18,   119,     0,
      28,    76,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   245,   246,   247,     0,     0,   178,   214,   217,   216,
     180,   215,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   196,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   213,   212,   218,   219,     0,     0,     0,
       0,     0,     0,   226,   230,     0,     0,    12,    13,     0,
       0,     0,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    63,     0,    19,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   233,   132,     0,   138,    77,
       0,    85,    87,   131,     0,    80,    82,   108,     0,    84,
       1,   250,   222,   125,   150,   127,     0,     0,   122,     0,
     120,     0,   224,     0,     0,     0,    87,     0,     0,   244,
     248,   179,   221,   220,     0,     0,    14,     0,     0,     0,
       5,   227,   231,    11,     8,     0,     0,    10,    62,    32,
      33,    34,    36,    37,    39,    40,    44,    45,    42,    43,
      47,    48,    50,    52,    54,    56,    58,     0,    75,     0,
     142,   140,   139,     0,    78,     0,   228,   255,     0,     0,
       0,   130,     0,     0,   111,     0,   124,     0,     0,   152,
     151,   153,   126,    21,     0,     0,     0,   117,   223,     0,
       0,     0,     0,     0,   243,     0,     0,     0,    30,     0,
       9,     7,     0,   133,   143,   141,    86,     0,   164,    88,
     229,   254,   137,   149,     0,   144,   145,   135,     0,     0,
     107,   112,     0,   114,   116,   160,     0,     0,   156,     0,
     154,     0,     0,   118,   123,   121,   234,   236,   238,     0,
       0,     0,   242,    15,   237,    29,    60,   167,     0,     0,
     147,   152,   148,   136,     0,   134,   106,     0,   113,   161,
     155,   157,   162,     0,   158,     0,     0,     0,   240,     0,
       0,   165,   146,   115,   163,   159,   235,   239,   241,   166,
     168
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    52,    53,   245,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,   183,
      69,   102,    70,   107,   210,   211,    72,    73,    74,    75,
     293,   294,   342,   343,    76,   229,   230,   295,    77,   236,
     213,   214,   282,   346,   335,   336,   226,   347,   301,   329,
     368,    78,    79,   155,   156,   157,    80,    81,    82,    83,
     288,   165,    84,    85,    86,    87,    88,    89,    90
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -310
static const yytype_int16 yypact[] =
{
     355,  -310,   -52,  -310,  -310,  1023,  1044,  1044,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,     8,  1056,   -44,
     -28,    35,    41,   662,    50,   128,    75,    86,    81,   785,
    1206,   122,   124,   820,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,   514,  -310,    45,   436,  1056,  -310,   -15,     1,   198,
      37,   171,    80,   120,   158,   233,    -1,  -310,  -310,   -25,
    -310,    27,  1391,  1391,  -310,    11,  -310,  1391,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,   440,  -310,
    -310,   662,  -310,   875,  -310,  1056,  -310,  -310,   164,   253,
    -310,  -310,   177,   662,  1056,  1056,  1056,    27,   208,   801,
     178,  -310,  -310,  -310,    32,   179,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  -310,  -310,  -310,  -310,   725,   896,  1056,
     197,   194,    92,  -310,  -310,   588,   267,  -310,  -310,   920,
    1056,   268,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310,  -310,  -310,  1056,  -310,  1056,  1056,  1056,  1056,  1056,
    1056,  1056,  1056,  1056,  1056,  1056,  1056,  1056,  1056,  1056,
    1056,  1056,  1056,  1056,  1056,  -310,  -310,    39,   -17,  -310,
      51,  -310,  1185,    65,    55,  -310,  -310,   185,  1411,  -310,
    -310,  -310,  -310,  1411,   108,  1411,   207,   253,   189,   -47,
    -310,   662,  -310,   112,   113,   132,   195,   216,   801,  -310,
    -310,  -310,  -310,  -310,   662,   134,  -310,   166,  1056,   219,
    -310,  -310,  -310,  -310,  -310,   169,    -4,  -310,  -310,  -310,
    -310,  -310,   -15,   -15,     1,     1,   198,   198,   198,   198,
      37,    37,   171,    80,   120,   158,   233,   -19,  -310,   222,
    -310,  -310,   -17,    39,  -310,   249,  -310,  -310,  1245,  1313,
     933,    65,  1411,   802,  -310,    39,  -310,  1268,   949,    73,
    -310,    99,  -310,  -310,   -42,  1056,   253,  -310,  -310,   662,
     662,   662,  1056,   970,  -310,   662,  1056,   205,  -310,  1056,
    -310,  -310,  1056,  -310,  -310,  -310,  -310,   249,  -310,  -310,
    -310,  -310,  -310,    36,   226,   223,  -310,  -310,   227,  1145,
    -310,  -310,    56,  -310,  -310,  -310,   228,   230,  -310,   232,
      99,  1352,  1007,  -310,  -310,  -310,   246,  -310,  -310,   175,
     662,   176,  -310,  -310,  -310,  -310,  -310,  -310,   -41,  1105,
    -310,    40,  -310,  -310,  1391,  -310,  -310,    39,  -310,  -310,
    -310,  -310,  -310,   235,  -310,   234,   662,   217,  -310,   662,
     119,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,  -310,
    -310
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -310,  -310,  -310,   -99,    28,  -310,   -51,    22,    57,   -45,
      63,   106,   107,   109,   110,   105,  -310,   -23,  -156,  -310,
     -26,  -102,  -197,     0,  -310,    25,  -310,   -55,  -310,  -310,
      18,  -271,  -310,   -65,  -310,    88,    10,   -84,   -49,   -64,
    -206,  -188,  -310,  -258,  -310,   -57,  -310,  -214,  -275,  -309,
    -310,   -32,  -310,  -310,   162,  -310,  -310,  -310,  -310,  -191,
    -310,  -310,   -33,  -310,  -310,  -310,  -310,   239,  -310
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      71,   108,   246,   246,   184,   101,   116,   212,   291,   224,
     300,    98,   114,   246,   217,   286,   202,   162,   367,   164,
     281,   287,   341,   306,   350,    22,    23,   258,   306,   390,
     206,   334,    91,    94,    96,    97,   299,   104,   223,   206,
     103,   307,   206,   206,   225,   204,   353,   391,   278,   192,
     193,   204,   166,   167,   168,   208,   100,   185,   206,   222,
     247,   205,   186,   187,   321,   322,   204,   162,   341,   162,
     255,   232,   215,   216,   188,   189,   238,   219,   233,   234,
     235,   400,   203,   100,    92,     3,     4,     5,    71,     6,
       7,   330,   207,   383,   325,    99,   350,   331,   218,   208,
     105,   369,   204,   298,   207,   369,   106,   298,   208,   299,
     169,   208,   170,   209,   171,   109,   194,   195,   240,   372,
     207,   283,    92,     3,     4,     5,   377,     6,     7,   328,
     289,   110,   290,   252,   259,   260,   261,   284,   297,   296,
     298,   302,   378,   279,   256,   371,    43,   266,   267,   268,
     269,   198,    44,    45,    46,    47,    48,    49,   250,   280,
     363,   111,   204,   223,   351,   291,   352,   113,   223,   225,
     223,   328,   112,   297,   225,   298,   225,   277,   309,   310,
     208,   371,   204,   204,    43,   196,   197,   158,   338,   159,
      44,    45,    46,    47,    48,    49,   349,   318,   311,   308,
     315,   199,   204,   354,   316,   313,   327,   399,   190,   191,
     262,   263,   314,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   344,   317,   324,   328,   320,   316,   223,   223,   316,
     200,   387,   389,   225,   225,   204,   204,   264,   265,   201,
     385,   227,    92,     3,     4,     5,   228,     6,     7,   270,
     271,   231,   237,   248,   239,   241,   249,   101,   365,   370,
     253,   257,   292,   303,   305,   101,   100,   356,   357,   358,
     285,   312,   101,   362,   223,   319,   359,   361,   323,   333,
     225,   364,   373,   374,   379,   375,   380,   333,   386,   366,
     381,   394,   395,   397,   272,   279,   273,   276,   326,   274,
     339,   275,   393,   344,    43,   304,   355,   392,   100,   243,
      44,    45,    46,    47,    48,    49,   100,   221,   388,   101,
       0,     0,     0,   100,     0,     0,   327,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   100,     0,     0,
     100,   333,     0,     0,   396,     1,     0,   398,     2,     3,
       4,     5,     0,     6,     7,     0,     0,     0,     0,   333,
       0,     0,     0,     0,   333,     0,     0,     0,     0,     0,
     100,     0,     0,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,     0,    28,    29,    30,     0,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,     0,     0,     0,     0,     0,    44,    45,    46,    47,
      48,    49,     0,     0,     0,     0,     0,     0,     0,     0,
     220,    50,    51,     2,     3,     4,     5,     0,     6,     7,
       0,     0,     0,     0,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,     0,     0,     0,     0,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,     0,    28,
      29,    30,     0,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,     0,     0,     0,     0,
       0,    44,    45,    46,    47,    48,    49,     2,     3,     4,
       5,   182,     6,     7,     0,     0,    50,    51,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,     0,    28,    29,    30,     0,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
       0,     0,     0,     0,     0,    44,    45,    46,    47,    48,
      49,     2,     3,     4,     5,     0,     6,     7,     0,     0,
      50,    51,   163,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,     0,    28,    29,    30,
       0,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,     0,     0,     0,     0,     0,    44,
      45,    46,    47,    48,    49,     2,     3,     4,     5,     0,
       6,     7,     0,     0,    50,    51,   251,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
       0,    28,    29,    30,     0,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,   117,   118,
     119,   120,     0,    44,    45,    46,    47,    48,    49,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    50,    51,
       0,     0,     0,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,     0,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,    92,     3,
       4,     5,     0,     6,     7,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    92,     3,     4,     5,     0,     6,
       7,   242,     0,   115,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    92,     3,     4,     5,     0,     6,     7,
       8,     0,     0,     0,     0,     0,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      43,     0,   160,     0,     0,     0,    44,    45,    46,    47,
      48,    49,     0,     0,   161,     0,    43,     0,     0,     0,
       0,    50,    44,    45,    46,    47,    48,    49,    92,     3,
       4,     5,     0,     6,     7,    43,     0,    50,     0,     0,
     340,    44,    45,    46,    47,    48,    49,     0,     0,    92,
       3,     4,     5,     8,     6,     7,     0,     0,     0,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    92,     3,     4,     5,     0,     6,     7,
       0,     0,     0,     0,     0,     0,    92,     3,     4,     5,
      43,     6,     7,     0,     0,     0,    44,    45,    46,    47,
      48,    49,    92,     3,     4,     5,     0,     6,     7,     0,
       0,    43,   244,     0,     0,     0,     0,    44,    45,    46,
      47,    48,    49,    92,     3,     4,     5,     0,     6,     7,
       0,     0,     0,     0,     0,    43,   254,     0,     0,     0,
       0,    44,    45,    46,    47,    48,    49,     0,    43,     0,
       0,   337,     0,     0,    44,    45,    46,    47,    48,    49,
      92,     3,     4,     5,    43,     6,     7,   348,     0,     0,
      44,    45,    46,    47,    48,    49,    92,     3,     4,     5,
       0,     6,     7,     0,     0,    43,   360,     0,     0,     0,
       0,    44,    45,    46,    47,    48,    49,    92,     3,     4,
       5,     0,     6,     7,     0,     0,     0,     0,     0,    92,
       3,     4,     5,     0,     6,     7,     0,     0,     0,     0,
       0,     0,    43,     0,     0,   384,     0,     0,    44,    45,
      46,    47,    48,    49,     0,     0,     0,     0,    93,     0,
       0,     0,     0,     0,    44,    45,    46,    47,    48,    49,
       0,     0,     0,     0,     0,     0,     0,     0,   206,    95,
       0,     0,     0,     0,     0,    44,    45,    46,    47,    48,
      49,    43,     0,     0,     0,     0,     0,    44,    45,    46,
      47,    48,    49,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     369,   345,   298,     8,     0,     0,     0,   208,     0,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   117,
     118,   119,   120,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,   376,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,     0,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,     0,
     285,     0,    51,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    51,   297,   345,   298,     0,     0,     0,     0,
     208,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   332,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   382,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,     8,
       0,     0,     0,     0,     0,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27
};

static const yytype_int16 yycheck[] =
{
       0,    33,   158,   159,    55,    28,    39,    71,   214,    93,
     224,     3,    38,   169,     3,   212,    17,    43,   327,    51,
     208,   212,   293,    70,   299,    42,    43,   183,    70,    70,
       3,   289,    84,     5,     6,     7,   224,    65,    93,     3,
      84,    88,     3,     3,    93,    70,    88,    88,   204,    12,
      13,    70,     7,     8,     9,    72,    28,    72,     3,    91,
     159,    86,    77,    78,    68,    84,    70,    93,   339,    95,
     169,   103,    72,    73,    73,    74,   109,    77,   104,   105,
     106,   390,    83,    55,     3,     4,     5,     6,    88,     8,
       9,   288,    65,   351,   282,    87,   371,   288,    87,    72,
      65,    65,    70,    67,    65,    65,    65,    67,    72,   297,
      65,    72,    67,    86,    69,    65,    79,    80,    86,   333,
      65,    70,     3,     4,     5,     6,    70,     8,     9,   285,
      65,     3,    67,   165,   185,   186,   187,    86,    65,   223,
      67,   225,    86,   207,   170,   333,    65,   192,   193,   194,
     195,    71,    71,    72,    73,    74,    75,    76,    66,   208,
     316,    86,    70,   218,    65,   371,    67,    86,   223,   218,
     225,   327,    86,    65,   223,    67,   225,   203,    66,    66,
      72,   369,    70,    70,    65,    14,    15,    65,   290,    65,
      71,    72,    73,    74,    75,    76,   298,   248,    66,   231,
      66,    81,    70,   305,    70,   238,    87,    88,    10,    11,
     188,   189,   244,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   295,    66,   282,   390,    66,    70,   292,   293,    70,
      82,    66,    66,   292,   293,    70,    70,   190,   191,    16,
     352,    87,     3,     4,     5,     6,     3,     8,     9,   196,
     197,    84,    54,    66,    86,    86,    72,   290,   319,   333,
       3,     3,    87,    66,    85,   298,   248,   309,   310,   311,
      85,    65,   305,   315,   339,    66,   312,   313,    66,   289,
     339,    86,    66,    70,    66,    68,    66,   297,    52,   322,
      68,    66,    68,    86,   198,   369,   199,   202,   283,   200,
     292,   201,   377,   377,    65,   227,   306,   374,   290,   157,
      71,    72,    73,    74,    75,    76,   298,    88,   360,   352,
      -1,    -1,    -1,   305,    -1,    -1,    87,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   319,    -1,    -1,
     322,   351,    -1,    -1,   386,     0,    -1,   389,     3,     4,
       5,     6,    -1,     8,     9,    -1,    -1,    -1,    -1,   369,
      -1,    -1,    -1,    -1,   374,    -1,    -1,    -1,    -1,    -1,
     352,    -1,    -1,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    49,    50,    51,    -1,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    -1,    -1,    -1,    -1,    -1,    71,    72,    73,    74,
      75,    76,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       0,    86,    87,     3,     4,     5,     6,    -1,     8,     9,
      -1,    -1,    -1,    -1,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    -1,    -1,    -1,    -1,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    49,
      50,    51,    -1,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    -1,    -1,    -1,    -1,
      -1,    71,    72,    73,    74,    75,    76,     3,     4,     5,
       6,    85,     8,     9,    -1,    -1,    86,    87,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    -1,    49,    50,    51,    -1,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      -1,    -1,    -1,    -1,    -1,    71,    72,    73,    74,    75,
      76,     3,     4,     5,     6,    -1,     8,     9,    -1,    -1,
      86,    87,    88,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    49,    50,    51,
      -1,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    -1,    -1,    -1,    -1,    -1,    71,
      72,    73,    74,    75,    76,     3,     4,     5,     6,    -1,
       8,     9,    -1,    -1,    86,    87,    88,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      -1,    49,    50,    51,    -1,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,     3,     4,
       5,     6,    -1,    71,    72,    73,    74,    75,    76,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    86,    87,
      -1,    -1,    -1,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,     3,     4,
       5,     6,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,    -1,     8,
       9,    86,    -1,    28,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,    -1,     8,     9,
      28,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      65,    -1,    32,    -1,    -1,    -1,    71,    72,    73,    74,
      75,    76,    -1,    -1,    44,    -1,    65,    -1,    -1,    -1,
      -1,    86,    71,    72,    73,    74,    75,    76,     3,     4,
       5,     6,    -1,     8,     9,    65,    -1,    86,    -1,    -1,
      88,    71,    72,    73,    74,    75,    76,    -1,    -1,     3,
       4,     5,     6,    28,     8,     9,    -1,    -1,    -1,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,     3,     4,     5,     6,    -1,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
      65,     8,     9,    -1,    -1,    -1,    71,    72,    73,    74,
      75,    76,     3,     4,     5,     6,    -1,     8,     9,    -1,
      -1,    65,    66,    -1,    -1,    -1,    -1,    71,    72,    73,
      74,    75,    76,     3,     4,     5,     6,    -1,     8,     9,
      -1,    -1,    -1,    -1,    -1,    65,    66,    -1,    -1,    -1,
      -1,    71,    72,    73,    74,    75,    76,    -1,    65,    -1,
      -1,    68,    -1,    -1,    71,    72,    73,    74,    75,    76,
       3,     4,     5,     6,    65,     8,     9,    68,    -1,    -1,
      71,    72,    73,    74,    75,    76,     3,     4,     5,     6,
      -1,     8,     9,    -1,    -1,    65,    66,    -1,    -1,    -1,
      -1,    71,    72,    73,    74,    75,    76,     3,     4,     5,
       6,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,    -1,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    65,    -1,    -1,    68,    -1,    -1,    71,    72,
      73,    74,    75,    76,    -1,    -1,    -1,    -1,    65,    -1,
      -1,    -1,    -1,    -1,    71,    72,    73,    74,    75,    76,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,    65,
      -1,    -1,    -1,    -1,    -1,    71,    72,    73,    74,    75,
      76,    65,    -1,    -1,    -1,    -1,    -1,    71,    72,    73,
      74,    75,    76,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      65,    66,    67,    28,    -1,    -1,    -1,    72,    -1,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    88,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    -1,
      85,    -1,    87,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    87,    65,    66,    67,    -1,    -1,    -1,    -1,
      72,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    28,
      -1,    -1,    -1,    -1,    -1,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     0,     3,     4,     5,     6,     8,     9,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    49,    50,
      51,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    71,    72,    73,    74,    75,    76,
      86,    87,    90,    91,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   109,
     111,   112,   115,   116,   117,   118,   123,   127,   140,   141,
     145,   146,   147,   148,   151,   152,   153,   154,   155,   156,
     157,    84,     3,    65,    93,    65,    93,    93,     3,    87,
      93,   106,   110,    84,    65,    65,    65,   112,   140,    65,
       3,    86,    86,    86,   109,    28,   151,     3,     4,     5,
       6,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,   142,   143,   144,    65,    65,
      32,    44,   109,    88,   140,   150,     7,     8,     9,    65,
      67,    69,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    85,   108,    95,    72,    77,    78,    73,    74,
      10,    11,    12,    13,    79,    80,    14,    15,    71,    81,
      82,    16,    17,    83,    70,    86,     3,    65,    72,    86,
     113,   114,   128,   129,   130,   112,   112,     3,    87,   112,
       0,   156,   140,   116,   126,   127,   135,    87,     3,   124,
     125,    84,   140,   109,   109,   109,   128,    54,   151,    86,
      86,    86,    86,   143,    66,    92,   107,    92,    66,    72,
      66,    88,   140,     3,    66,    92,   109,     3,   107,    95,
      95,    95,    96,    96,    97,    97,    98,    98,    98,    98,
      99,    99,   100,   101,   102,   103,   104,   109,   107,   128,
     127,   130,   131,    70,    86,    85,   111,   148,   149,    65,
      67,   129,    87,   119,   120,   126,   126,    65,    67,   130,
     136,   137,   126,    66,   124,    85,    70,    88,   140,    66,
      66,    66,    65,   151,   140,    66,    70,    66,    95,    66,
      66,    68,    84,    66,   127,   130,   114,    87,   107,   138,
     111,   148,    66,   112,   132,   133,   134,    68,   110,   119,
      88,   120,   121,   122,   128,    66,   132,   136,    68,   110,
     137,    65,    67,    88,   110,   125,   140,   140,   140,   109,
      66,   109,   140,   107,    86,    95,   106,   138,   139,    65,
     128,   130,   136,    66,    70,    68,    88,    70,    86,    66,
      66,    68,    66,   132,    68,   110,    52,    66,   140,    66,
      70,    88,   134,   122,    66,    68,   140,    86,   140,    88,
     138
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (&yylloc, yyscanner, cparser, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc, yyscanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, yyscanner, cparser); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, yyscan_t yyscanner, cint_cparser_t* cparser)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, yyscanner, cparser)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    yyscan_t yyscanner;
    cint_cparser_t* cparser;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (yyscanner);
  YYUSE (cparser);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, yyscan_t yyscanner, cint_cparser_t* cparser)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, yyscanner, cparser)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    yyscan_t yyscanner;
    cint_cparser_t* cparser;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, yyscanner, cparser);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, yyscan_t yyscanner, cint_cparser_t* cparser)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, yyscanner, cparser)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    yyscan_t yyscanner;
    cint_cparser_t* cparser;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , yyscanner, cparser);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, yyscanner, cparser); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, yyscan_t yyscanner, cint_cparser_t* cparser)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, yyscanner, cparser)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    yyscan_t yyscanner;
    cint_cparser_t* cparser;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (yyscanner);
  YYUSE (cparser);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}






struct yypstate
  {
        /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.
       `yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[2];

    YYSIZE_T yystacksize;

    /* Used to determine if this is the first time this instance has
       been used.  */
    int yynew;
  };

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (yyscan_t yyscanner, cint_cparser_t* cparser)
#else
int
yyparse (yyscanner, cparser)
    yyscan_t yyscanner;
    cint_cparser_t* cparser;
#endif
{
  return yypull_parse (0, yyscanner, cparser);
}

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yypull_parse (yypstate *yyps, yyscan_t yyscanner, cint_cparser_t* cparser)
#else
int
yypull_parse (yyps, yyscanner, cparser)
    yypstate *yyps;
    yyscan_t yyscanner;
    cint_cparser_t* cparser;
#endif
{
  int yystatus;
  yypstate *yyps_local;
  int yychar;
  YYSTYPE yylval = 0;
  YYLTYPE yylloc = {0,0,0,0};
  if (yyps == 0)
    {
      yyps_local = yypstate_new ();
      if (!yyps_local)
        {
          yyerror (&yylloc, yyscanner, cparser, YY_("memory exhausted"));
          return 2;
        }
    }
  else
    yyps_local = yyps;
  do {
    yychar = YYLEX;
    yystatus =
      yypush_parse (yyps_local, yychar, &yylval, &yylloc, yyscanner, cparser);
  } while (yystatus == YYPUSH_MORE);
  if (yyps == 0)
    yypstate_delete (yyps_local);
  return yystatus;
}

/* Initialize the parser data structure.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
yypstate *
yypstate_new (void)
#else
yypstate *
yypstate_new ()

#endif
{
  yypstate *yyps;
  yyps = (yypstate *) YYMALLOC (sizeof *yyps);
  if (!yyps)
    return 0;
  yyps->yynew = 1;
  return yyps;
}

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void
yypstate_delete (yypstate *yyps)
#else
void
yypstate_delete (yyps)
    yypstate *yyps;
#endif
{
#ifndef yyoverflow
  /* If the stack was reallocated but the parse did not complete, then the
     stack still needs to be freed.  */
  if (!yyps->yynew && yyps->yyss != yyps->yyssa)
    YYSTACK_FREE (yyps->yyss);
#endif
  YYFREE (yyps);
}

#define cint_c_nerrs yyps->cint_c_nerrs
#define yystate yyps->yystate
#define yyerrstatus yyps->yyerrstatus
#define yyssa yyps->yyssa
#define yyss yyps->yyss
#define yyssp yyps->yyssp
#define yyvsa yyps->yyvsa
#define yyvs yyps->yyvs
#define yyvsp yyps->yyvsp
#define yylsa yyps->yylsa
#define yyls yyps->yyls
#define yylsp yyps->yylsp
#define yyerror_range yyps->yyerror_range
#define yystacksize yyps->yystacksize

/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yypush_parse (yypstate *yyps, int yypushed_char, YYSTYPE const *yypushed_val, YYLTYPE const *yypushed_loc, yyscan_t yyscanner, cint_cparser_t* cparser)
#else
int
yypush_parse (yyps, yypushed_char, yypushed_val, yypushed_loc, yyscanner, cparser)
    yypstate *yyps;
    int yypushed_char;
    YYSTYPE const *yypushed_val;
    YYLTYPE const *yypushed_loc;
    yyscan_t yyscanner;
    cint_cparser_t* cparser;
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval = 0;

/* Location data for the lookahead symbol.  */
YYLTYPE yylloc = {0,0,0,0};


  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  if (!yyps->yynew)
    {
      yyn = yypact[yystate];
      goto yyread_pushed_token;
    }

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;

#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 1;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);

	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
	YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      if (!yyps->yynew)
        {
          YYDPRINTF ((stderr, "Return for a new token:\n"));
          yyresult = YYPUSH_MORE;
          goto yypushreturn;
        }
      yyps->yynew = 0;
yyread_pushed_token:
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yypushed_char;
      if (yypushed_val)
        yylval = *yypushed_val;
      if (yypushed_loc)
        yylloc = *yypushed_loc;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 101 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 102 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 103 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 104 "cint_grammar.y"
    { (yyval) = (yyvsp[(2) - (3)]); }
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 108 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 109 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpOpenBracket, (yyvsp[(1) - (4)]), (yyvsp[(3) - (4)])); }
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 111 "cint_grammar.y"
    {
              (yyval) = cint_ast_function((yyvsp[(1) - (3)]), 0); 
          }
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 115 "cint_grammar.y"
    {
              (yyval) = cint_ast_function((yyvsp[(1) - (4)]), (yyvsp[(3) - (4)])); 
          }
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 119 "cint_grammar.y"
    { 
              (yyval) = cint_ast_operator(cintOpDot, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); 
          }
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 123 "cint_grammar.y"
    { 
              (yyval) = cint_ast_operator(cintOpArrow, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); 
          }
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 126 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpIncrement, (yyvsp[(1) - (2)]), 0); }
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 127 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpDecrement, (yyvsp[(1) - (2)]), 0); }
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 133 "cint_grammar.y"
    {
              cint_ast_append((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); 
              (yyval) = (yyvsp[(1) - (3)]); 
          }
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 140 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 141 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpIncrement, 0, (yyvsp[(2) - (2)])); }
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 142 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpDecrement, 0, (yyvsp[(2) - (2)])); }
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 143 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cint_ast_int((yyvsp[(1) - (2)])), 0, (yyvsp[(2) - (2)])); }
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 144 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpSizeof, 0, (yyvsp[(2) - (2)])); }
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 145 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpSizeof, 0, (yyvsp[(3) - (4)])); }
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 149 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpAddressOf); }
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 150 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpDereference); }
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 151 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpPositive); }
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 152 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpNegative); }
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 153 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpTilde); }
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 154 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpNot); }
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 161 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpTypecast, CINT_AST_PTR_VOID, (yyvsp[(5) - (5)])); }
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 163 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpTypecast, CINT_AST_PTR_AUTO, (yyvsp[(4) - (4)])); }
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 168 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpMultiply, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 169 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpDivide, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 170 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpMod, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 175 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpAdd, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 176 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpSubtract, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 181 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpLeftShift, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 182 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpRightShift, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 187 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpLessThan, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 188 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpGreaterThan, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 189 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpLessThanOrEqual, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 190 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpGreaterThanOrEqual, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 195 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpEqual, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 196 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpNotEqual, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 201 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpBitwiseAnd, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 206 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpBitwiseXor, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 211 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpBitwiseOr, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 216 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpLogicalAnd, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 221 "cint_grammar.y"
    { (yyval) = cint_ast_operator(cintOpLogicalOr, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 227 "cint_grammar.y"
    { (yyval) = cint_ast_ternary((yyvsp[(1) - (5)]), (yyvsp[(3) - (5)]), (yyvsp[(5) - (5)])); }
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 233 "cint_grammar.y"
    {     
              /*
               * Arithmetic assignment operators are converted to distinct operator/= expressions
               * for the benefit of simplifying the interpreter operator logic. 
               *
               * Convert any expressions of the form "x <op>= y" to "x = x <op> y"; 
               * 
               */
              (yyval) = cint_ast_operator(cint_ast_int((yyvsp[(2) - (3)])), (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); 
              if(cint_ast_int((yyvsp[(2) - (3)])) != cintOpAssign) {
                  /* Perform the operator on the left and right and assign it to the left */
                  (yyval) = cint_ast_operator(cintOpAssign, (yyvsp[(1) - (3)]), (yyval)); 
              }
          }
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 250 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpAssign); }
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 251 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpMultiply); }
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 252 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpDivide); }
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 253 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpMod); }
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 254 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpAdd); }
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 255 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpSubtract); }
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 256 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpLeftShift); }
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 257 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpRightShift); }
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 258 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpBitwiseAnd); }
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 259 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpBitwiseXor); }
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 260 "cint_grammar.y"
    { (yyval) = cint_ast_integer(cintOpBitwiseOr); }
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 264 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 266 "cint_grammar.y"
    {
              (yyval) = cint_ast_comma((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
          }
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 276 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (2)]); }
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 278 "cint_grammar.y"
    {
              (yyval) = cint_ast_declaration_init((yyvsp[(1) - (3)]), (yyvsp[(2) - (3)]));
          }
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 284 "cint_grammar.y"
    {(yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 285 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); (yyval) = (yyvsp[(1) - (2)]); }
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 286 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 287 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); (yyval) = (yyvsp[(1) - (2)]); }
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 289 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); (yyval) = (yyvsp[(1) - (2)]); }
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 294 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); (yyval) = (yyvsp[(1) - (3)]); }
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 298 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 300 "cint_grammar.y"
    { (yyval) = cint_ast_declarator_init((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 304 "cint_grammar.y"
    { (yyval) = cint_ast_integer(CINT_AST_TYPE_F_EXTERN); }
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 305 "cint_grammar.y"
    { (yyval) = cint_ast_integer(CINT_AST_TYPE_F_TYPEDEF); }
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 306 "cint_grammar.y"
    { (yyval) = cint_ast_integer(CINT_AST_TYPE_F_STATIC); }
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 312 "cint_grammar.y"
    { (yyval) = cint_ast_type("void"); }
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 313 "cint_grammar.y"
    { (yyval) = cint_ast_type("char"); }
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 314 "cint_grammar.y"
    { (yyval) = cint_ast_type("short"); }
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 315 "cint_grammar.y"
    { (yyval) = cint_ast_type("int"); }
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 316 "cint_grammar.y"
    { (yyval) = cint_ast_type("long"); }
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 317 "cint_grammar.y"
    { (yyval) = cint_ast_type("float"); }
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 318 "cint_grammar.y"
    { (yyval) = cint_ast_type("double"); }
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 319 "cint_grammar.y"
    { (yyval) = cint_ast_integer(CINT_AST_TYPE_F_SIGNED); }
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 320 "cint_grammar.y"
    { (yyval) = cint_ast_integer(CINT_AST_TYPE_F_UNSIGNED); }
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 323 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 327 "cint_grammar.y"
    { (yyval) = cint_ast_structure_def((yyvsp[(2) - (5)]), (yyvsp[(4) - (5)])); }
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 328 "cint_grammar.y"
    { (yyval) = cint_ast_structure_def(0, (yyvsp[(3) - (4)])); }
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 339 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); (yyval) = (yyvsp[(1) - (2)]); }
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 344 "cint_grammar.y"
    { 
          (yyval) = cint_ast_struct_declaration((yyvsp[(1) - (3)]), (yyvsp[(2) - (3)])); 
      }
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 352 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); (yyval) = (yyvsp[(1) - (3)]); }
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 363 "cint_grammar.y"
    { (yyval) = cint_ast_enumdef((yyvsp[(2) - (5)]), (yyvsp[(4) - (5)])); }
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 368 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 369 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); (yyval) = (yyvsp[(1) - (3)]); }
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 373 "cint_grammar.y"
    { (yyval) = cint_ast_enumerator((yyvsp[(1) - (1)]), 0); }
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 374 "cint_grammar.y"
    { (yyval) = cint_ast_enumerator((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); }
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 378 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); (yyval) = (yyvsp[(1) - (2)]); }
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 379 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 380 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); (yyval) = (yyvsp[(1) - (2)]); }
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 381 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 385 "cint_grammar.y"
    { (yyval) = cint_ast_integer(CINT_AST_TYPE_F_CONST); }
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 386 "cint_grammar.y"
    { (yyval) = cint_ast_integer(CINT_AST_TYPE_F_VOLATILE); }
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 391 "cint_grammar.y"
    { 
            (yyval) = cint_ast_pointer_declarator((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); 
        }
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 394 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 398 "cint_grammar.y"
    { (yyval) = cint_ast_identifier_declarator((yyvsp[(1) - (1)])); }
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 401 "cint_grammar.y"
    { (yyval) = cint_ast_array_declarator((yyvsp[(1) - (4)]), (yyvsp[(3) - (4)])); }
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 403 "cint_grammar.y"
    { (yyval) = cint_ast_array_declarator((yyvsp[(1) - (3)]), cint_ast_integer(-1)); }
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 405 "cint_grammar.y"
    {
              (yyval) = cint_ast_function_declarator((yyvsp[(1) - (4)]), (yyvsp[(3) - (4)])); 
          }
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 410 "cint_grammar.y"
    { 
              (yyval) = cint_ast_function_declarator((yyvsp[(1) - (3)]), 0); 
          }
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 416 "cint_grammar.y"
    { (yyval) = cint_ast_integer(1); }
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 417 "cint_grammar.y"
    { (yyval) = cint_ast_integer(1); }
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 418 "cint_grammar.y"
    { (yyval) = cint_ast_pointer_indirect((yyvsp[(2) - (2)])); }
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 424 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); (yyval) = (yyvsp[(1) - (2)]); }
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 434 "cint_grammar.y"
    {(yyval) = (yyvsp[(1) - (1)]);}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 436 "cint_grammar.y"
    {
              cint_ast_append((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); 
              (yyval) = (yyvsp[(1) - (3)]); 
          }
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 444 "cint_grammar.y"
    {     
              (yyval) = cint_ast_parameter_declaration_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); 
          }
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 449 "cint_grammar.y"
    {
              (yyval) = cint_ast_parameter_declaration((yyvsp[(1) - (1)]));               
          }
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 486 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 487 "cint_grammar.y"
    { (yyval) = cint_ast_initializer((yyvsp[(2) - (3)])); }
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 488 "cint_grammar.y"
    { (yyval) = cint_ast_initializer((yyvsp[(2) - (4)])); }
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 493 "cint_grammar.y"
    { cint_ast_append((yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); (yyval) = (yyvsp[(1) - (3)]); }
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 500 "cint_grammar.y"
    {
              (yyval) = cint_ast_compound_statement((yyvsp[(1) - (1)])); 
          }
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 506 "cint_grammar.y"
    {(yyval)=(yyvsp[(1) - (1)]);}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 514 "cint_grammar.y"
    { (yyval) = cint_ast_print((yyvsp[(2) - (2)])); }
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 515 "cint_grammar.y"
    { (yyval) = cint_ast_print((yyvsp[(2) - (3)])); }
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 519 "cint_grammar.y"
    { (yyval) = cint_ast_string("sizeof"); }
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 520 "cint_grammar.y"
    { (yyval) = cint_ast_string("typedef"); }
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 521 "cint_grammar.y"
    { (yyval) = cint_ast_string("extern"); }
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 522 "cint_grammar.y"
    { (yyval) = cint_ast_string("static"); }
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 523 "cint_grammar.y"
    { (yyval) = cint_ast_string("auto"); }
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 524 "cint_grammar.y"
    { (yyval) = cint_ast_string("register"); }
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 525 "cint_grammar.y"
    { (yyval) = cint_ast_string("char"); }
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 526 "cint_grammar.y"
    { (yyval) = cint_ast_string("short"); }
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 527 "cint_grammar.y"
    { (yyval) = cint_ast_string("int"); }
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 528 "cint_grammar.y"
    { (yyval) = cint_ast_string("long"); }
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 529 "cint_grammar.y"
    { (yyval) = cint_ast_string("signed"); }
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 530 "cint_grammar.y"
    { (yyval) = cint_ast_string("unsigned"); }
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 531 "cint_grammar.y"
    { (yyval) = cint_ast_string("float"); }
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 532 "cint_grammar.y"
    { (yyval) = cint_ast_string("double"); }
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 533 "cint_grammar.y"
    { (yyval) = cint_ast_string("const"); }
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 534 "cint_grammar.y"
    { (yyval) = cint_ast_string("volatile"); }
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 535 "cint_grammar.y"
    { (yyval) = cint_ast_string("void"); }
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 536 "cint_grammar.y"
    { (yyval) = cint_ast_string("struct"); }
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 537 "cint_grammar.y"
    { (yyval) = cint_ast_string("union"); }
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 538 "cint_grammar.y"
    { (yyval) = cint_ast_string("enum"); }
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 539 "cint_grammar.y"
    { (yyval) = cint_ast_string("case"); }
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 540 "cint_grammar.y"
    { (yyval) = cint_ast_string("default"); }
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 541 "cint_grammar.y"
    { (yyval) = cint_ast_string("if"); }
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 542 "cint_grammar.y"
    { (yyval) = cint_ast_string("else"); }
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 543 "cint_grammar.y"
    { (yyval) = cint_ast_string("switch"); }
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 544 "cint_grammar.y"
    { (yyval) = cint_ast_string("while"); }
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 545 "cint_grammar.y"
    { (yyval) = cint_ast_string("do"); }
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 546 "cint_grammar.y"
    { (yyval) = cint_ast_string("for"); }
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 547 "cint_grammar.y"
    { (yyval) = cint_ast_string("goto"); }
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 548 "cint_grammar.y"
    { (yyval) = cint_ast_string("continue"); }
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 549 "cint_grammar.y"
    { (yyval) = cint_ast_string("break"); }
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 550 "cint_grammar.y"
    { (yyval) = cint_ast_string("return"); }
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 551 "cint_grammar.y"
    { (yyval) = cint_ast_string("cint"); }
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 552 "cint_grammar.y"
    { (yyval) = cint_ast_string("print"); }
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 568 "cint_grammar.y"
    {
              cint_ast_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); 
              (yyval) = (yyvsp[(1) - (2)]); 
          }
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 575 "cint_grammar.y"
    { (yyval) = cint_ast_cint((yyvsp[(2) - (3)])); }
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 579 "cint_grammar.y"
    { (yyval) = cint_ast_case((yyvsp[(2) - (4)]), (yyvsp[(4) - (4)])); }
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 580 "cint_grammar.y"
    { (yyval) = cint_ast_case(0, (yyvsp[(3) - (3)])); }
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 587 "cint_grammar.y"
    { (yyval) = cint_ast_empty(); }
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 588 "cint_grammar.y"
    { (yyval) = (yyvsp[(2) - (3)]); }
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 594 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 595 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (2)]); cint_ast_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); }
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 599 "cint_grammar.y"
    { (yyval) = (yyvsp[(1) - (1)]); }
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 601 "cint_grammar.y"
    {
              cint_ast_append((yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); 
              (yyval) = (yyvsp[(1) - (2)]); 
          }
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 608 "cint_grammar.y"
    { (yyval) = cint_ast_empty(); }
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 614 "cint_grammar.y"
    {
              (yyval) = cint_ast_if((yyvsp[(3) - (5)]), (yyvsp[(5) - (5)]), 0); 
          }
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 618 "cint_grammar.y"
    {
              (yyval) = cint_ast_if((yyvsp[(3) - (7)]), (yyvsp[(5) - (7)]), (yyvsp[(7) - (7)])); 
          }
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 621 "cint_grammar.y"
    { (yyval) = cint_ast_switch((yyvsp[(3) - (5)]), (yyvsp[(5) - (5)])); }
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 622 "cint_grammar.y"
    { (yyval) = cint_interpreter_macro((yyvsp[(1) - (5)]), (yyvsp[(3) - (5)])); }
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 627 "cint_grammar.y"
    {
              (yyval) = cint_ast_while((yyvsp[(3) - (5)]), (yyvsp[(5) - (5)]), 0); 
          }
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 631 "cint_grammar.y"
    {
              (yyval) = cint_ast_while((yyvsp[(5) - (7)]), (yyvsp[(2) - (7)]), 1); 
          }
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 635 "cint_grammar.y"
    {
              (yyval) = cint_ast_for((yyvsp[(3) - (6)]), (yyvsp[(4) - (6)]), 0, (yyvsp[(6) - (6)])); 
          }
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 639 "cint_grammar.y"
    {
              (yyval) = cint_ast_for((yyvsp[(3) - (7)]), (yyvsp[(4) - (7)]), (yyvsp[(5) - (7)]), (yyvsp[(7) - (7)])); 
          }
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 643 "cint_grammar.y"
    {
              (yyval) = cint_interpreter_iterator((yyvsp[(1) - (5)]), (yyvsp[(3) - (5)]), (yyvsp[(5) - (5)])); 
          }
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 647 "cint_grammar.y"
    {
              (yyval) = cint_interpreter_iterator((yyvsp[(1) - (4)]), 0, (yyvsp[(3) - (4)])); 
          }
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 654 "cint_grammar.y"
    { (yyval) = cint_ast_continue(); }
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 655 "cint_grammar.y"
    { (yyval) = cint_ast_break(); }
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 656 "cint_grammar.y"
    { (yyval) = cint_ast_return(0); }
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 657 "cint_grammar.y"
    { (yyval) = cint_ast_return((yyvsp[(2) - (3)])); }
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 663 "cint_grammar.y"
    { 
            /* Accept and append this translation unit */
            if(cparser->result) {
                cint_ast_append(cparser->result, (yyvsp[(1) - (1)])); 
            }   
            else {
                cparser->result = (yyvsp[(1) - (1)]); 
            }
            
            if(yychar == YYEMPTY) {
                /* 
                 * This unit is complete and we have no dangling lookahead. 
                 * Return to the application. 
                 */
                YYACCEPT; 
            }
            else {
                /*
                 * A lookahead has been consumed. If we were to return from parsing
                 * now we would lose the lookahead on the next invokation. 
                 * 
                 * We will continue to parse units until it is safe to return. 
                 */
            }       
        }
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 689 "cint_grammar.y"
    {
            /* Accept and append this translation unit */
            if(cparser->result) {
                cint_ast_append(cparser->result, (yyvsp[(2) - (2)])); 
            }   
            else {
                cparser->result = (yyvsp[(1) - (2)]); 
            }

            if(yychar == YYEMPTY) {
                /* 
                 * This unit is complete and we have no dangling lookahead. 
                 * Return to the application. 
                 */
                YYACCEPT; 
            }
            else {
                /*
                 * A lookahead has been consumed. If we were to return from parsing
                 * now we would lose the lookahead on the next invokation. 
                 * 
                 * We will continue to parse units until it is safe to return. 
                 */
            }       
        }
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 715 "cint_grammar.y"
    {
            cparser->result = 0; 
            YYACCEPT; 
        }
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 731 "cint_grammar.y"
    {
              (yyval) = cint_ast_function_definition((yyvsp[(1) - (3)]), (yyvsp[(2) - (3)]), (yyvsp[(3) - (3)])); 
          }
    break;



/* Line 1455 of yacc.c  */
#line 3725 "cint_c.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, yyscanner, cparser, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (&yylloc, yyscanner, cparser, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, yyscanner, cparser, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc, yyscanner, cparser);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, yyscanner, cparser);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, yyscanner, cparser, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, yyscanner, cparser);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, yyscanner, cparser);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  yyps->yynew = 1;

yypushreturn:
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 738 "cint_grammar.y"


#include "cint_porting.h"

void cint_c_error(YYLTYPE* locp, yyscan_t yyscanner, cint_cparser_t* cp, const char* msg)
{
    const int sourceLineLen = LONGEST_SOURCE_LINE;      /* Truncate source lines longer than 256 characters. */
    char sourceLine[LONGEST_SOURCE_LINE];
    char errLine[LONGEST_SOURCE_LINE];
    char *errPtr = errLine;
    int errCol;
    int tokLen;
    int i;
    char *currentFileName;
    int currentLineNum;

    (void) cint_current_line(yyscanner, sourceLine, sourceLineLen, &errCol,
                             &tokLen, &currentFileName, &currentLineNum);
    if (sourceLine[0] && !cint_cparser_interactive()) {
        /* Print current source line (if there is one) and not interactive */
        CINT_PRINTF("%s\n", sourceLine);
    }
    if (tokLen) {
        /* Create a "marker" line pointing to offending token. */
        for (i = 0; i < errCol; i++) {
            *errPtr++ = ' ';
        }
        for (i = 0; i < tokLen; i++) {
            *errPtr++ = '^';
        }
        *errPtr = 0;
	errPtr = errLine;
    } else {
        /* If tokLen is zero, either the line was longer than max allowed or current line is empty. */
        errPtr = "[No current line]";
    }

    /* print marker and parser message */
    if (currentFileName) {
        /* If file name is NULL, we're at the top */
        CINT_PRINTF("%s %s [%s:%d]\n", errPtr, msg, currentFileName, currentLineNum);
    } else if (currentLineNum) {
        CINT_PRINTF("%s %s [%d]\n", errPtr, msg, currentLineNum);
    } else {
        CINT_PRINTF("%s %s\n", errPtr, msg);
    }
}


#else /* CINT_CONFIG_INCLUDE_PARSER */
int cint_grammar_c_not_empty;
#endif

