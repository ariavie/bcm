;
; $Id: sdk56634.asm 1.8 Broadcom SDK $
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
; This is the default program for the (49+4 port) BCM56634 SDKs.
; To start it, use the following commands from BCM:
;
;       led load sdk56634.hex
;       led auto on
;       led start
;
; The is 1 LEDs per XE port. 
;
; There is one bit per 10 gigabit Ethernet LED with the following colors:
;       ONE 		Black
;       ZERO            Green
;
; Each chip drives only its own LEDs and needs to write them in the order:
;       A01/L01, A02/L02, ..., A04/L04
;
; Link up/down info cannot be derived from LINKEN or LINKUP, as the LED
; processor does not always have access to link status.  This program
; assumes link status is kept current in bit 0 of RAM byte (0xA0 + portnum).
; Generally, a program running on the main CPU must update these
; locations on link change; see linkscan callback in
; $SDK/src/appl/diag/ledproc.c.
;
; Current implementation:
;
; L01/A01 reflects port 1 link/activity status:
;       Black: no link
;       Green: Link
;       Black: no link
;       Green: Activity
;
;
NUM_PORT        EQU     4

; Device ports
; 26 hg0     
; 27 hg1     
; 28 hg2     
; 29 hg3     

PORT_HG0        EQU     26
PORT_HG1        EQU     27
PORT_HG2        EQU     28
PORT_HG3        EQU     29

; The TX/RX activity lights will be extended for ACT_EXT_TICKS
; so they will be more visible.
TICKS           EQU     1
SECOND_TICKS    EQU     (30*TICKS)
ACT_EXT_TICKS   EQU     (SECOND_TICKS/3)
TXRX_ALT_TICKS  EQU     (SECOND_TICKS/6)

;
; Main Update Routine
;
;  This routine is called once per tick.
;
update:
	ld	a, PORT_HG0
        port    a
        call    link_activity        ; Off if no link

	ld	a, PORT_HG1
        port    a
        call    link_activity        ; Off if no link

	ld	a, PORT_HG2
        port    a
        call    link_activity        ; Off if no link

	ld	a, PORT_HG3
        port    a
        call    link_activity        ; Off if no link

        inc     (TXRX_ALT_COUNT)

        send    NUM_PORT * 2 

;
; link_activity
;
;  This routine calculates the activity LED for the current port.
;  It extends the activity lights using timers (new activity overrides
;  and resets the timers).
;
;  Inputs: (PORT_NUM)
;  Outputs: one bit sent to LED stream
;

link_activity:
        ;jmp     led_green       ;DEBUG ONLY
        call    get_link
        jnc     link_led_black
        call    led_green
        jmp     chk_activity
link_led_black:
        call    led_black
        jmp     led_black
chk_activity:
        port    a
        pushst  RX
        pushst  TX
        tor
        pop

        jnc     led_black       ; Always black, No Activity

        ;jmp     led_green       ;DEBUG ONLY

        ld      b, (TXRX_ALT_COUNT)
        and     b, TXRX_ALT_TICKS 
        jnz     led_green
        jmp     led_black

link_status:
        ;jmp     led_black       ;DEBUG ONLY
        call    get_link
        jnc     led_black
        jmp     led_green

;
; get_link
;
;  This routine finds the link status LED for a port.
;  Link info is in bit 0 of the byte read from PORTDATA[port]
;
;  Inputs: Port number in a
;  Outputs: Carry flag set if link is up, clear if link is down.
;  Destroys: a, b
;

get_link:
        ld      b,PORTDATA
        add     b,a
        ld      b,(b)
        tst     b,0
        ret

;
; led_black, led_green
;
;  Inputs: None
;  Outputs: one bit  to the LED stream indicating color
;  Destroys: None
;

led_black:
        pushst  ONE
        pack
        ret

led_green:
        pushst  ZERO
        pack
        ret

;
; Variables (SDK software initializes LED memory from 0xa0-0xff to 0)
;

TXRX_ALT_COUNT  equ     0xe0
PORT_NUM        equ     0xe1

;
; Port data, which must be updated continually by main CPU's
; linkscan task.  See $SDK/src/appl/diag/ledproc.c for examples.
; In this program, bit 0 is assumed to contain the link up/down status.
;

;Offset 0x1e00 - 0x80
;LED scan chain assembly area

;Offset 0x1e80 - 0xa0
PORTDATA        equ     0xa0    ; Size 48 + 1 + 4 bytes

;
; Symbolic names for the bits of the port status fields
;

RX              equ     0x0     ; received packet
TX              equ     0x1     ; transmitted packet
COLL            equ     0x2     ; collision indicator
SPEED_C         equ     0x3     ; 100 Mbps
SPEED_M         equ     0x4     ; 1000 Mbps
DUPLEX          equ     0x5     ; half/full duplex
FLOW            equ     0x6     ; flow control capable
LINKUP          equ     0x7     ; link down/up status
LINKEN          equ     0x8     ; link disabled/enabled status
ZERO            equ     0xE     ; always 0
ONE             equ     0xF     ; always 1
