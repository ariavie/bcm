;
; $Id: generic8led.asm 1.4 Broadcom SDK $
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
; This is a generic program that controls 8 LEDs
; To start it, use the following commands from BCM:
;
;       led load generic8led.hex
;       led auto on
;       led start
;
; There are 8 LEDs -
;      6 LEDs to display port number and two LEDs for TX/RX activity
;
; There is one bit per LED with the following colors:
;       ONE 		Black
;       ZERO            Green
;
; Each chip drives only its own LEDs and needs to write them in the order:
;       6-bit port number, Activity-TX-Port, Activity-RX-Port
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
; TA01/RA01 reflects port N TX/RX activity status:
;       Black: no Activity
;       Green: Activity
;
;
NUM_LEDS        EQU     8

MIN_PORT        EQU     0
MAX_PORT        EQU     63
NUM_PORT        EQU     64

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
        ld      a,(PORT_NUM)
        cmp     a, NUM_PORT
        jc      led_update_port

        sub     a,a             ; ld a,MIN_PORT (but only one instr)
        ld      (PORT_NUM),a
        ld      b, (SKIP_ZERO)
        cmp     b, 1
        jc      skip_link_check ; Turn off all LEDs if no port has link
        ld      (SKIP_ZERO), a

led_update_port:
        call    get_link
        jnc     next_port       ; Skip ports that don't have link

skip_link_check:
        ld      b, 1
        ld      (SKIP_ZERO), b
        call    display_port_num
        port    a
        call    tx_rx_activity        ; Off if no link

        inc     (TXRX_ALT_COUNT)

        ld      b, (PORT_ALT_COUNT)
        inc     b
        ld      (PORT_ALT_COUNT), b
        cmp     b, 1 * SECOND_TICKS
        jnc     next_port
        send    NUM_LEDS

next_port:
        sub     b,b
        ld      (PORT_ALT_COUNT),b
        inc     (PORT_NUM)
        jmp     update

;
; link_activity
;
;  This routine calculates the activity LED for the current port.
;
;  Inputs: (PORT_NUM)
;  Outputs: one bit sent to LED stream
;

tx_rx_activity:
        ;jmp     led_green       ;DEBUG ONLY
        call    get_link
        jnc     link_led_black
        jmp     chk_activity
link_led_black:
        call    led_black
        jmp     led_black
chk_activity:
        call    chk_activity_tx
        call    chk_activity_rx
        ret

chk_activity_tx:
        port    a
        pushst  TX
        pop

        jnc     led_black       ; Always black, No Activity

        ld      b, (TXRX_ALT_COUNT)
        and     b, TXRX_ALT_TICKS 
        jnz     led_green
        jmp     led_black

chk_activity_rx:
        port    a
        pushst  RX
        pop

        jnc     led_black       ; Always black, No Activity

        ld      b, (TXRX_ALT_COUNT)
        and     b, TXRX_ALT_TICKS 
        jnz     led_green
        jmp     led_black

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

;  Inputs: Port number in a
;        : Port bit position in b
display_port_one_bit:
        tst     a, b
        jnc     led_black
        jmp     led_green

;  Inputs: Port number in a
;  Destroys: b
display_port_num:
        ld      b, 5
        call    display_port_one_bit

        ld      b, 4
        call    display_port_one_bit

        ld      b, 3
        call    display_port_one_bit

        ld      b, 2
        call    display_port_one_bit

        ld      b, 1
        call    display_port_one_bit

        ld      b, 0
        call    display_port_one_bit

        ret

;
; Variables (SDK software initializes LED memory from 0xa0-0xff to 0)
;

TXRX_ALT_COUNT  equ     0xe0
PORT_NUM        equ     0xe1
PORT_ALT_COUNT  equ     0xe2
SKIP_ZERO       equ     0xe3

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
