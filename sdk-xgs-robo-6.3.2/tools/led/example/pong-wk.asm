;
; $Id: pong-wk.asm 1.4 Broadcom SDK $
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
; This is the silly Pong program for the White Knight 5690.
; To start it, use the following commands from BCM:
;
;	led load pong-wk.hex
;	led auto on
;	led start
;

TICKS		EQU	1
SECOND_TICKS	EQU	(30*TICKS)
UPDATE_TICKS	EQU	(SECOND_TICKS/10)

POS_X		EQU	0xa0
POS_Y		EQU	0xa1
DIR_X		EQU	0xa2
DIR_Y		EQU	0xa3
TIMER		EQU	0xa4

update:
	ld	a,(DIR_X)
	cmp	a,0
	jnz	up1
	; If DIR variable is 0, it tells us to do first time init
	ld	a,1
	ld	(DIR_X),a
	ld	(DIR_Y),a
up1:	
	; Display ball at position (POS_X, POS_Y)

	ld	a,0		; column

update_col:
	cmp	a,(POS_X)
	jz	up3
	call	led_black
	call	led_black
	call	led_black
	call	led_black
	jmp	next_col
up3:
	ld	b,(POS_Y)	; 1st row
	cmp	b,0
	jnz	up4
	call	led_green
	jmp	up5	
up4:
	call	led_black
up5:

	dec	b		; 2nd row
	jnz	up6
	call	led_green
	jmp	up7
up6:
	call	led_black
up7:

	dec	b		; 3rd row
	jnz	up8
	call	led_green
	jmp	up9
up8:
	call	led_black
up9:
	dec	b		; 4th row
	jnz	up10
	call	led_green
	jmp	up11
up10:
	call	led_black
up11:

next_col:
	inc	a
	cmp	a,12
	jc	update_col

	; Update position if timer expires

	inc	(TIMER)
	ld	a,(TIMER)
	cmp	a,UPDATE_TICKS
	jnz	skip_update
	sub	a,a
	ld	(TIMER),a

	ld	a,(POS_X)
	add	a,(DIR_X)
	ld	(POS_X),a
	cmp	a,0
	jnz	up20
	ld	a,1
	ld	(DIR_X),a
	jmp	up21
up20:
	cmp	a,5
	jnz	up21
	ld	a,-1
	ld	(DIR_X),a
up21:

	ld	a,(POS_Y)
	add	a,(DIR_Y)
	ld	(POS_Y),a
	cmp	a,0
	jnz	up22
	ld	a,1
	ld	(DIR_Y),a
	jmp	up23
up22:
	cmp	a,3
	jnz	up23
	ld	a,-1
	ld	(DIR_Y),a
up23:

skip_update:

	send	48

;
; led_black, led_amber, led_green
;
;  Inputs: None
;  Outputs: Two bits to the LED stream indicating color
;  Destroys: None
;

led_black:
	pushst	ZERO
	pack
	pushst	ZERO
	pack
	ret

led_green:
	pushst	ZERO
	pack
	pushst	ONE
	pack
	ret

led_amber:
	pushst	ONE
	pack
	pushst	ZERO
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
