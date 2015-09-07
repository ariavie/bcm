# -*- Makefile -*-
# $Id: Makefile,v 1.1 Broadcom SDK $
# $Copyright: Copyright 2012 Broadcom Corporation.
# This program is the proprietary software of Broadcom Corporation
# and/or its licensors, and may only be used, duplicated, modified
# or distributed pursuant to the terms and conditions of a separate,
# written license agreement executed between you and Broadcom
# (an "Authorized License").  Except as set forth in an Authorized
# License, Broadcom grants no license (express or implied), right
# to use, or waiver of any kind with respect to the Software, and
# Broadcom expressly reserves all rights in and to the Software
# and all intellectual property rights therein.  IF YOU HAVE
# NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
# IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
# ALL USE OF THE SOFTWARE.  
#  
# Except as expressly set forth in the Authorized License,
#  
# 1.     This program, including its structure, sequence and organization,
# constitutes the valuable trade secrets of Broadcom, and you shall use
# all reasonable efforts to protect the confidentiality thereof,
# and to use this information only in connection with your use of
# Broadcom integrated circuit products.
#  
# 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
# PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
# REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
# OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
# DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
# NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
# ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
# CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
# OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
# 
# 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
# BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
# INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
# ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
# TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
# THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
# WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
# ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
#
# API mode Makefile
#

ifneq (,$(findstring apimode,$(MAKECMDGOALS)))
LBUILD=1
endif

ifndef LBUILD

# Build as part of the SDK

LOCALDIR = src/appl/diag/api

include ${SDK}/make/Make.config

CFGFLAGS += -I$(SDK)/src/appl/cint
CFGFLAGS += -I$(SDK)/src/appl/diag/cint

lib = libdiagapi
include ${SDK}/make/Make.lib

include ${SDK}/make/Make.depend
else

# Build standalone interpreter application

CFLAGS := -g
CFLAGS += -DYYDEBUG
CFLAGS += -DAPIMODE_INCLUDE_MAIN=1
CFLAGS += -Wall
CFLAGS += -Werror 
CFLAGS += -Wformat
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wuninitialized
CFLAGS += -Wunreachable-code
CFLAGS += -Winline
CFLAGS += -Wpointer-sign
CFLAGS += -Wextra
CFLAGS += -Wmissing-format-attribute
CFLAGS += -Wunused-parameter
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wshadow
CFLAGS += -Wbad-function-cast
CFLAGS += -Wlogical-op
CFLAGS += -Waggregate-return
CFLAGS += -Wold-style-definition
CFLAGS += -Wmissing-prototypes
CFLAGS += -Wmissing-declarations
CFLAGS += -Wmissing-noreturn
CFLAGS += -Wnested-externs
CFLAGS += -ansi
CFLAGS += -I../../../../include
CFLAGS += -I../../../appl/cint
CFLAGS += -DVA_COPY_PTR
CFLAGS += -DCINT_CONFIG_INCLUDE_STDLIB=1
CFLAGS += -D_GNU_SOURCE

LDFLAGS := -g
CC := /tools/bin/gcc
BISON := /tools/bin/bison
YEAR := $(shell date +%Y)
TMP := tmp.h

apimode: main

gen: api_grammar.tab.c

main.o: api_mode.h api_grammar.tab.h

api_mode.o: api_mode.h api_grammar.tab.h

main: main.o api_grammar.tab.o api_mode.o

api_grammar.tab.h: api_grammar.tab.c

api_grammar.tab.c: api_grammar.y api_mode.h
	$(BISON) -g -d -v $<
	@echo '/* ' > $(TMP)
	@echo -n '   $$' >> $(TMP)
	@echo -n 'Id: ' >> $(TMP)
	@echo '$$' >> $(TMP)
	@echo '   $$Copyright: $(YEAR) Broadcom Corp.' >> $(TMP)
	@echo '   All Rights Reserved.$$' >> $(TMP)
	@echo '*/' >> $(TMP)
	@cat api_grammar.tab.h >> $(TMP)
	@rm api_grammar.tab.h
	@mv tmp.h api_grammar.tab.h

grammar: api_grammar.tab.c

clean:
	-rm -f api_grammar.output \
                api_grammar.tab.* *.o *.dot main


endif
