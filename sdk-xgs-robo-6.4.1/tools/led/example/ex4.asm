; $Id: ex4.asm 1.3 Broadcom SDK $
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
;Here is an example program for programming the LEDs on the 5605.
;
;The situation is this:
;
;    26 ports (port 0-25) each have three LEDs.
;          led 0: RX
;          led 1: TX
;          led 2: speed
;		black if disabled
;		blinking amber if  10 Mbps but link down or disabled
;		solid on amber if  10 Mbps and link up
;		blinking green if 100 Mbps but link down or disabled
;		solid on green if 100 Mbps and link up
;                [ ports 24 and 25 are the gig ports and are slightly
;                  different; they are green for 1G, and amber for
;                  anything less, either 10Mbps or 100Mbps ].
;
;    Bicolor LEDs are really two LEDs with two inputs: with one LED
;    of the pair driven, the LED is green; with the other one driven,
;    the LED is red, with both driven, it is amber (yellow).
;
;    The scan out order is RX, TX, inp1, inp2 for port 0, then
;    the same for port 1, and so on through port 25.  Note that
;    ports 24 and 25 need slightly different handling than the
;    other ports.
;
;
;-------------------------- start of program --------------------------

begin:
	; update refresh phase within 1 Hz
	ld	A, phase
	inc	A		; next phase
	cmp	A, 30		; see if 1 sec has gone by
	jc	chkblink
	sub	A, A		; A = 0

chkblink:
	ld	(phase),A
	sub	B, B		; B = 0
	cmp	A, 15		; A already has the phase
	jnc	noblink
	inc	B		; B = 1
noblink:
	ld	(blink),B
	
	sub	A,A		; start with port 0
	ld	(portnum),A

portloop:
	port	A		; specify which port we're dealing with

	pushst	RX		; first put out RX status
	pack			; send to output buffer

	pushst	TX		; next put out RX status
	pack			; send to output buffer

	; here's the tricky part.  to encode bicolor LED speed status,
	; we do the following:
	;    inp1 inp2
	;     0    0    = off
	;     1    0    = green = full speed
	;     1    1    = amber = less than full speed
	;     0    1    = red   = not used

	pushst	LINKEN		; link enabled

	push	(blink)
	tand			; turn off LED if blinking
	pop			; cy = LINKEN & blink
	push	cy		; put it back
	pack			; inp1

	push	cy		; tos = LINKEN & blink
	ld	A,(portnum)
	cmp	A,24
	jnc	dogigs
	pushst	SPEED_C		; test for 100Mbps speed
	jmp	wrapup
; ports 24 and 25 are the gigabit ports
dogigs:
	pushst	SPEED_M		; test for 1000Mbps speed

wrapup:
	tinv			; make it a test for not full speed
	tand			; combine with LINKEN & blink
	pack			; inp2

	inc	(portnum)
	ld	A,(portnum)
	cmp	A,26
	jnz	portloop

	send	104		; send 26*4 LED pulses and halt


; data storage
blink		equ	0x7D	; 1 if blink on, 0 if blink off

phase		equ	0x7E	; phase within 30 Hz
				; should be initialized to 0 on reset

portnum		equ	0x7F	; temp to hold which port we're working on


; symbolic labels
; this gives symbolic names to the various bits of the port status fields

RX		equ	0x0	; received packet
TX		equ	0x1	; transmitted packet
COLL		equ	0x2	; collision indicator
SPEED_C		equ	0x3	;  100 Mbps
SPEED_M		equ	0x4	; 1000 Mbps
DUPLEX		equ	0x5	; half/full duplex
FLOW		equ	0x6	; flow control capable
LINKUP		equ	0x7	; link down/up status
LINKEN		equ	0x8	; link disabled/enabled status
ZERO		equ	0xE	; always 0
ONE		equ	0xF	; always 1

;-------------------------- end of program --------------------------
;
;This program is 62 bytes in length, but it is fairly complicatd.

