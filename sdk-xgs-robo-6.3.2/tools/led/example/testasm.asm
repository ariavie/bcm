; $Id: testasm.asm 1.5 Broadcom SDK $
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
; this contains one specimen from each of the opcodes.
; any byte that needs a second byte will use a value equal to the opcode.

	ld	a,a		; 0x00
	ld	a,b		; 0x01
	ld	a,0x02		; 0x02
				; 0x03
	ld	a,(a)		; 0x04
	ld	a,(b)		; 0x05
	ld	a,(6)		; 0x06
	clc			; 0x07
	tst	a,a		; 0x08
	tst	a,b		; 0x09
	tst	a,0x0A		; 0x0A
	bit	a,a		; 0x0C
	bit	a,b		; 0x0D
	bit	a,14		; 0x0E
				; 0x0F

	ld	b,a		; 0x10
	ld	b,b		; 0x11
	ld	b,0x12		; 0x12
				; 0x13
	ld	b,(a)		; 0x14
	ld	b,(b)		; 0x15
	ld	b,(22)		; 0x16
	stc			; 0x17
	tst	b,a		; 0x18
	tst	b,b		; 0x19
	tst	b,0x1A		; 0x1A
	bit	b,a		; 0x1C
	bit	b,b		; 0x1D
	bit	b,30		; 0x1E
				; 0x1F

	push	a		; 0x20
	push	b		; 0x21
	push	0x22		; 0x22
				; 0x23
	push	(a)		; 0x24
	push	(b)		; 0x25
	push	(38)		; 0x26
	push	cy		; 0x27
	port	a		; 0x28
	port	b		; 0x29
	port	0x2A		; 0x2A
				; 0x2B
	port	(a)		; 0x2C
	port	(b)		; 0x2D
	port	(46)		; 0x2E
				; 0x2F

	pushst	a		; 0x30
	pushst	b		; 0x31
	pushst	0x32		; 0x32
				; 0x33
				; 0x34
				; 0x35
				; 0x36
	cmc			; 0x37
	send	a		; 0x38
	send	b		; 0x39
	send	0x3A		; 0x3A
				; 0x3B
	send	(a)		; 0x3C
	send	(b)		; 0x3D
	send	(076)		; 0x3E
				; 0x3F

	ld	(a),a		; 0x40
	ld	(a),b		; 0x41
	ld	(a),0x42	; 0x42
				; 0x43
	ld	(a),(a)		; 0x44
	ld	(a),(b)		; 0x45
	ld	(a),(70)	; 0x46
				; 0x47
	tst	(a),a		; 0x48
	tst	(a),b		; 0x49
	tst	(a),0x4A	; 0x4A
				; 0x4B
	bit	(a),a		; 0x4C
	bit	(a),b		; 0x4D
	bit	(a),78		; 0x4E
				; 0x4F

	ld	(b),a		; 0x50
	ld	(b),b		; 0x51
	ld	(b),0x52	; 0x52
				; 0x53
	ld	(b),(a)		; 0x54
	ld	(b),(b)		; 0x55
	ld	(b),(86)	; 0x56
	ret			; 0x57
	tst	(b),a		; 0x58
	tst	(b),b		; 0x59
	tst	(b),0x5A	; 0x5A
				; 0x5B
	bit	(b),a		; 0x5C
	bit	(b),b		; 0x5D
	bit	(b),94		; 0x5E
				; 0x5F

	ld	(0x60),a	; 0x60
	ld	(0x61),b	; 0x61
;	ld	(0x62),0x62	; 0x62  -- 3 bytes!
				; 0x63
	ld	(0x64),(a)	; 0x64
	ld	(0x65),(b)	; 0x65
;	ld	(0x66),(70)	; 0x66  -- 3 bytes!
	call	0x67		; 0x67
	tst	(0x68),a	; 0x68
	tst	(0x69),b	; 0x69
;	tst	(0x6A),0x4A	; 0x6A  -- 3 bytes!
				; 0x6B
	bit	(0x6C),a	; 0x6C
	bit	(0x6D),b	; 0x6D
;	bit	(0x6E),78	; 0x6E  -- 3 bytes!
				; 0x6F

	jz	0x70		; 0x70
	jc	0x71		; 0x71
	jt	0x72		; 0x72
				; 0x73
	jnz	0x74		; 0x74
	jnc	0x75		; 0x75
	jnt	0x76		; 0x76
	jmp	0x77		; 0x77
				; 0x77
				; 0x78
				; 0x79
				; 0x7A
				; 0x7B
				; 0x7C
				; 0x7D
				; 0x7E
				; 0x7F

	inc	a		; 0x80
	inc	b		; 0x81
				; 0x82
				; 0x83
	inc	(a)		; 0x84
	inc	(b)		; 0x85
	inc	(0x86)		; 0x86
	pack			; 0x87
	rol	a		; 0x88
	rol	b		; 0x89
				; 0x8A
				; 0x8B
	rol	(a)		; 0x8C
	rol	(b)		; 0x8D
	rol	(0x8E)		; 0x8E
				; 0x8F

	dec	a		; 0x90
	dec	b		; 0x91
				; 0x92
				; 0x93
	dec	(a)		; 0x94
	dec	(b)		; 0x95
	dec	(0x96)		; 0x96
	pop			; 0x97
	ror	a		; 0x98
	ror	b		; 0x99
				; 0x9A
				; 0x9B
	ror	(a)		; 0x9C
	ror	(b)		; 0x9D
	ror	(0x9E)		; 0x9E
				; 0x9F

	xor	a,a		; 0xA0
	xor	a,b		; 0xA1
	xor	a,0xA2		; 0xA2
				; 0xA3
	xor	a,(a)		; 0xA4
	xor	a,(b)		; 0xA5
	xor	a,(0xA6)	; 0xA6
	txor			; 0xA7
	xor	b,a		; 0xA8
	xor	b,b		; 0xA9
	xor	b,0xAA		; 0xAA
				; 0xAB
	xor	b,(a)		; 0xAC
	xor	b,(b)		; 0xAD
	xor	b,(0xAE)	; 0xAE
				; 0xAF

	or	a,a		; 0xB0
	or	a,b		; 0xB1
	or	a,0xB2		; 0xB2
				; 0xB3
	or	a,(a)		; 0xB4
	or	a,(b)		; 0xB5
	or	a,(0xB6)	; 0xB6
	tor			; 0xB7
	or	b,a		; 0xB8
	or	b,b		; 0xB9
	or	b,0xBA		; 0xBA
				; 0xBB
	or	b,(a)		; 0xBC
	or	b,(b)		; 0xBD
	or	b,(0xBE)	; 0xBE
				; 0xBF

	and	a,a		; 0xC0
	and	a,b		; 0xC1
	and	a,0xC2		; 0xC2
				; 0xC3
	and	a,(a)		; 0xC4
	and	a,(b)		; 0xC5
	and	a,(0xC6)	; 0xC6
	tand			; 0xC7
	and	b,a		; 0xC8
	and	b,b		; 0xC9
	and	b,0xCA		; 0xCA
				; 0xCB
	and	b,(a)		; 0xCC
	and	b,(b)		; 0xCD
	and	b,(0xCE)	; 0xCE
				; 0xCF

	cmp	a,a		; 0xD0
	cmp	a,b		; 0xD1
	cmp	a,0xD2		; 0xD2
	       			; 0xD3
	cmp	a,(a)		; 0xD4
	cmp	a,(b)		; 0xD5
	cmp	a,(0xD6)	; 0xD6
	tinv   			; 0xD7
	cmp	b,a		; 0xD8
	cmp	b,b		; 0xD9
	cmp	b,0xDA		; 0xDA
	      	 		; 0xDB
	cmp	b,(a)		; 0xDC
	cmp	b,(b)		; 0xDD
	cmp	b,(0xDE)	; 0xDE
	       			; 0xDF

	sub	a,a		; 0xE0
	sub	a,b		; 0xE1
	sub	a,0xE2		; 0xE2
	       			; 0xE3
	sub	a,(a)		; 0xE4
	sub	a,(b)		; 0xE5
	sub	a,(0xE6)	; 0xE6
	       			; 0xE7
	sub	b,a		; 0xE8
	sub	b,b		; 0xE9
	sub	b,0xEA		; 0xEA
	       			; 0xEB
	sub	b,(a)		; 0xEC
	sub	b,(b)		; 0xED
	sub	b,(0xEE)	; 0xEE
	       			; 0xEF

	add	a,a		; 0xF0
	add	a,b		; 0xF1
	add	a,0xF2		; 0xF2
	       			; 0xF3
	add	a,(a)		; 0xF4
	add	a,(b)		; 0xF5
	add	a,(0xF6)	; 0xF6
				; 0xF7
	add	b,a		; 0xF8
	add	b,b		; 0xF9
	add	b,0xFA		; 0xFA
				; 0xFB
	add	b,(a)		; 0xFC
	add	b,(b)		; 0xFD
	add	b,(0xFE)	; 0xFE
				; 0xFF
