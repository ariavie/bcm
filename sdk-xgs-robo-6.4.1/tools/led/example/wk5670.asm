;
; $Id: wk5670.asm 1.5 Broadcom SDK $
;
; $Copyright: Copyright 2012 Broadcom Corporation.
; This program is the proprietary software of Broadcom Corporation
; and/or its licensors, and may only be used, duplicated, modified
; or distributed pursuant to the terms and conditions of a separate,
; written license agreement executed between you and Broadcom
; (an "Authorized License").  Except as set forth in an Authorized
; License, Broadcom grants no license (express or implied), right
; to use, or waiver of any kind with respect to the Software, and
; Broadcom expressly reserves all rights in and to the Software
; and all intellectual property rights therein.  IF YOU HAVE
; NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
; IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
; ALL USE OF THE SOFTWARE.  
;  
; Except as expressly set forth in the Authorized License,
;  
; 1.     This program, including its structure, sequence and organization,
; constitutes the valuable trade secrets of Broadcom, and you shall use
; all reasonable efforts to protect the confidentiality thereof,
; and to use this information only in connection with your use of
; Broadcom integrated circuit products.
;  
; 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
; PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
; REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
; OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
; DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
; NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
; ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
; CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
; OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
; 
; 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
; BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
; INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
; ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
; TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
; THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
; WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
; ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
; 
;
; This is the default program for the White Knight 5670.
;
; To start it, use the following commands from BCM,
; where unit 0 is the 5670 and units 1 and 2 are the 5690s:
;
;	0:led load wk5670.hex
;	1:led load wk5690.hex
;	1:led auto on
;	2:led load wk5690.hex
;	2:led auto on
;	*:led start
;
; The 3 programs should be started all at once so that blinking
; is in sync across the 5690s.
;
; The White Knight has 14 columns of 4 LEDs each as shown below:
;
;            5690 Unit 1              5690 Unit 2          5670 Unit 0
;       -----------------------  -----------------------  --------------
;       L01 L03 L05 L07 L09 L11  L01 L03 L05 L07 L09 L11     E1 E2
;       A01 A03 A05 A07 A09 A11  A01 A03 A05 A07 A09 A11     L1 L2
;       L02 L04 L06 L08 L10 L12  L02 L04 L06 L08 L10 L12     T1 T2
;       A02 A04 A06 A08 A10 A12  A02 A04 A06 A08 A10 A12     R1 R2
;
; This program runs only on the 5670 and controls only the rightmost
; two columns of LEDs.  The wk5690 program runs on each 5690.
;
; There is one bit per LED with the following colors:
;	ZERO		Green
;	ONE		Black
;
; The bits are shifted out in the following order:
;	E1, L1, T1, R1, E2, L2, T2, R2
;
; Current implementation:
;	E1 reflects port 1 higig link enable
;	L1 reflects port 1 higig link up
;	T1 reflects port 1 higig transmit activity
;	R1 reflects port 1 higig receive activity
;
; Note:
;	To provide consistent behavior, the gigabit LED patterns match
;	those in sdk5605.asm and the higig LEDs match those in sdk5670.asm.
;
;       E1 E2
;       L1 L2
;       T1 T2
;       R1 R2

	port	3		; 5670 port 3 (right)
	call	put

	port	6		; 5670 port 6 (left)
	call	put

	send	8

put:
	pushst	LINKEN
	tinv
	pack
	pushst	LINKUP
	tinv
	pack
	pushst	TX
	tinv
	pack
	pushst	RX
	tinv
	pack
	ret

;
; Symbolic names for the bits of the port status fields
;

RX		equ	0x0	; received packet
TX		equ	0x1	; transmitted packet
COLL		equ	0x2	; collision indicator
SPEED_C		equ	0x3	; 100 Mbps
SPEED_M		equ	0x4	; 1000 Mbps
DUPLEX		equ	0x5	; half/full duplex
FLOW		equ	0x6	; flow control capable
LINKUP		equ	0x7	; link down/up status
LINKEN		equ	0x8	; link disabled/enabled status
ZERO		equ	0xE	; always 0
ONE		equ	0xF	; always 1
