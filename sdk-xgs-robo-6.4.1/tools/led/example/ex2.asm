; $Id: ex2.asm 1.3 Broadcom SDK $
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
;          led 2: COLL
;
;    The scan out order is RX, TX, COLL status for port 0, then
;    for port 1, ...
;
;
;-------------------------- start of program --------------------------

begin:
	ld	A,0		; start with port 0
	ld	(portnum),A

portloop:
	port	A		; specify which port we're dealing with
	pushst	RX
	pack			; send to output buffer
	pushst	TX
	pack			; send to output buffer
	pushst	COLL
	pack			; send to output buffer

	inc	(portnum)
	ld	A,(portnum)
	cmp	A,26
	jnz	portloop

	send	78		; send 26*3 LED pulses and halt
	

; data storage
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

;-------------------------- end of program --------------------------
;
;This program is 24 bytes in length and is only slightly more complicated
;than Example #1.
