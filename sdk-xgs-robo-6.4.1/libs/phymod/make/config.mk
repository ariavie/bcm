# $Id$
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
# PHYMOD make rules and definitions
#

#
# Provide reasonable defaults for configuration variables
#

# Default build directory
ifndef PHYMOD_BLDDIR
PHYMOD_BLDDIR = $(PHYMOD)/build
endif

# Location to build objects in
ifndef PHYMOD_OBJDIR
PHYMOD_OBJDIR = $(PHYMOD_BLDDIR)/obj
endif
override BLDDIR := $(PHYMOD_OBJDIR)

# Location to place libraries
ifndef PHYMOD_LIBDIR
PHYMOD_LIBDIR = $(PHYMOD_BLDDIR)
endif
LIBDIR := $(PHYMOD_LIBDIR)

# Option to retrieve compiler version
ifndef PHYMOD_CC_VERFLAGS
PHYMOD_CC_VERFLAGS := -v
endif
CC_VERFLAGS = $(PHYMOD_CC_VERFLAGS); 

# Default suffix for object files
ifndef PHYMOD_OBJSUFFIX
PHYMOD_OBJSUFFIX = o
endif
OBJSUFFIX = $(PHYMOD_OBJSUFFIX)

# Default suffix for library files
ifndef PHYMOD_LIBSUFFIX
PHYMOD_LIBSUFFIX = a
endif
LIBSUFFIX = $(PHYMOD_LIBSUFFIX)

#
# Set up compiler options, etc.
#

# Default include path
PHYMOD_INCLUDE_PATH = -I$(PHYMOD)/include

# Import preprocessor flags avoiding include duplicates
TMP_PHYMOD_CPPFLAGS := $(filter-out $(PHYMOD_INCLUDE_PATH),$(PHYMOD_CPPFLAGS))

# Convenience Makefile flags for building specific chips
ifdef PHYMOD_CHIPS
PHYMOD_DSYM_CPPFLAGS := -DPHYMOD_CONFIG_INCLUDE_CHIP_DEFAULT=0 
PHYMOD_DSYM_CPPFLAGS += $(foreach chip,$(PHYMOD_CHIPS),-DPHYMOD_CONFIG_INCLUDE_${chip}=1) 
endif
ifdef PHYMOD_NCHIPS
PHYMOD_DSYM_CPPFLAGS += $(foreach chip,$(PHYMOD_NCHIPS),-DPHYMOD_CONFIG_INCLUDE_${chip}=0)
endif

TMP_PHYMOD_CPPFLAGS += $(PHYMOD_DSYM_CPPFLAGS)
export PHYMOD_DSYM_CPPFLAGS

ifdef DSYMS
TMP_PHYMOD_CPPFLAGS += -DPHYMOD_CONFIG_CHIP_SYMBOLS_USE_DSYMS=1
endif

override CPPFLAGS = $(TMP_PHYMOD_CPPFLAGS) $(PHYMOD_INCLUDE_PATH)


# Import compiler flags
override CFLAGS = $(PHYMOD_CFLAGS)




#
# Define standard targets, etc.
#

ifdef LOCALDIR
override BLDDIR := $(BLDDIR)/$(LOCALDIR)
endif

ifndef LSRCS
LSRCS = $(wildcard *.c)
endif
ifndef LOBJS
LOBJS = $(addsuffix .$(OBJSUFFIX), $(basename $(LSRCS)))
endif
ifndef BOBJS
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))
endif

# Use PHYMOD_QUIET=1 to control printing of compilation lines.
ifdef PHYMOD_QUIET
Q = @
endif

#
# Define rules for creating object directories
#

.PRECIOUS: $(BLDDIR)/.tree

%/.tree:
	@$(ECHO) 'Creating build directory $(dir $@)'
	$(Q)$(MKDIR) $(dir $@)
	@$(ECHO) "Build Directory for $(LOCALDIR) created" > $@

#
# Configure build tools
#
include $(PHYMOD)/make/maketools.mk
