;
; $Id: twolynx.asm 1.5 Broadcom SDK $
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
; This is the default program for the Lynx SDK (BCM95673K4S)
;
; To start it, use the following commands from BCM:
;
;	0:led load twolynx.hex
;	1:led load twolynx.hex
;	*:led start
;
; On the Dual Lynx SDK board, there are two Lynx chips (unit 0 and 1).
; Each is connected to its own scan chain of 8 bits long.
;
; The Dual Lynx SDK has 4 columns of 4 LEDs each, as shown below:
;
;      E1 E2 E3 E4
;      R1 R2 R3 R4
;      T1 T2 T3 T4
;      L1 L2 L3 L4
;
; There is one bit per LED with the following colors:
;	ZERO		Green
;	ONE		Black
;
; The bits are shifted out in the following order:
;	E1, R1, T1, L1, E2, R2, T2, L2
;
; Current implementation:
;	E1 reflects port 1 higig link enable
;	R1 reflects port 1 higig receive activity
;	T1 reflects port 1 higig transmit activity
;	L1 reflects port 1 higig link up
;

	port	1
	call	put
	port	0
	call	put

	send	8

put:
	pushst	LINKEN
	tinv
	pack
	pushst	RX
	tinv
	pack
	pushst	TX
	tinv
	pack
	pushst	LINKUP
	tinv
	pack
	ret

TICKS		EQU	1
SECOND_TICKS	EQU	(30*TICKS)
TXRX_ALT_TICKS	EQU	(SECOND_TICKS/6)
TX_ALT_COUNT	equ	0xc0
RX_ALT_COUNT	equ	0xc1

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
