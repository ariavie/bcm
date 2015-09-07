# $Id: Makefile 1.42 Broadcom SDK $
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
# Compile sample LED driver programs.
#
# Notes:
#	The Merlin runs the same programs as White Knight (wk5670, wk5690)
#
.KEEP_STATE:

.SUFFIXES:	.asm .hex

ASM = ../tools/ledasm

OBJS = \
	ex1.hex \
	ex2.hex \
	ex3.hex \
	ex4.hex \
	ex5.hex \
	testasm.hex \
	sdk5605.hex \
	pong5605.hex \
	pong-wk.hex \
	sdk5670.hex \
	sdk5690.hex \
	p48.hex \
	knigget.hex \
	wk5670.hex \
	wk5690.hex \
	merlin5670.hex \
	merlin5690.hex \
	lm5690.hex \
	galahad.hex \
	lancelot.hex \
	k12-5690.hex \
	twolynx.hex \
	herculynx.hex \
	mirror.hex \
	sdk5665.hex \
	sdk5650.hex \
	guenevere5670.hex \
	guenevere5673.hex \
	sdk5675.hex \
	tuc24_ref.hex \
	lm48p5695.hex \
	lm48p5695_10.hex \
	sdk56504.hex \
	sdk56504ref.hex \
	lm48p56504.hex \
	bcm953300p24ref.hex \
	bcm956314p24ref.hex \
	lm48p56504_50.hex \
	poe48p56504.hex \
	sdk56102.hex \
	felix48.hex \
	lm12pcx456501.hex \
	sdk56601.hex \
	sdk56602.hex \
	sdk56800.hex \
	sdk56800c.hex \
	sdk56580.hex \
	lm12p56802.hex \
	sdk56018.hex \
	sdk56214.hex \
	sdk56680.hex \
	sdk56685.hex \
	sdk56820.hex \
	sdk56524.hex \
	sdk56624.hex \
	sdk56626.hex \
	sdk56628.hex \
	sdk56629.hex \
	sdk56634.hex \
	sdk56636.hex \
	sdk56638.hex \
	sdk56639.hex \
	bcm956820r24xg.hex \
	bcm956024p48ref.hex \
	bcm956024r50t.hex \
	generic8led.hex \
	sdk56840.hex \
	sdk56643ge.hex \
	sdk56640xe.hex \
	sdk56640hsp.hex \
	sdk56648hsp.hex \
	magnum.hex \
	sdk88025.hex \
	sdk88030ceup0.hex \
	sdk88030ceup1.hex \
	sdk88030geup0.hex \
	sdk88030geup1.hex \
	sdk88030xeup0.hex \
	sdk88650.hex
  

all: $(OBJS)

remake: clobber all

%.hex: %.asm
	$(ASM) $*

clean:
	rm -f *.lst

clobber: clean
	rm -f *.hex
	rm -f *.c
