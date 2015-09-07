;
; $Id: mirror.asm 1.4 Broadcom SDK $
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
; This LED program simply mirrors bits from the LED data area directly to
; the LED scan chain, effectively allowing the host CPU to write directly
; to the LEDs as memory-mapped bits.
;
;
; After this program is downloaded and starts executing, it increments
; the byte at address 0x80 once per frame.  This may help the host
; synchronize writes to frame updates, if needed.
;
; Next, the user program should write the length of the scan chain in
; bits to location 0x81.  This only needs to be done once.
;
; Next, the actual LED bits may be updated by writing to bytes to
; addresses 0x82 and above:
;
;	Byte 0x82 bits 7:0 are the first 8 bits on the scan chain
;	Byte 0x83 bits 7:0 are the second 8 bits on the scan chain
;	Etc.
;
; Remember that:
;	LED Data address 0x80 corresponds to PCI offset 0x1e00 (word write).
;	LED Data address 0x81 corresponds to PCI offset 0x1e04 (word write).
;	Etc.
;

BYTE_PTR	equ	0x7f

FRAME_COUNT	equ	0x80
BIT_COUNT	equ	0x81
BIT_DATA	equ	0x82

;
; Main Update Routine
;
;  This routine is called once per tick.
;

update:
	inc	(FRAME_COUNT)

	ld	a,BIT_DATA
	ld	(BYTE_PTR),a

	ld	b,(BIT_COUNT)

loop:
	ld	a,(BYTE_PTR)
	inc	(BYTE_PTR)
	ld	a,(a)

	call	shift_bit
	call	shift_bit
	call	shift_bit
	call	shift_bit
	call	shift_bit
	call	shift_bit
	call	shift_bit
	call	shift_bit

	cmp	b,0
	jnz	loop

	ld	b,(BIT_COUNT)
	send	b

;
; shift_bit
;	A: current byte
;	B: number of bits remaining
;
; Never shifts more than 'B' bits.
;

shift_bit:
	cmp	b,0
	jnz	sb1
	ret
sb1:
	dec	b
	tst	a,7
	ror	a
	jnc	sb2
	pushst	ZERO
	pack
	ret
sb2:
	pushst	ONE
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
