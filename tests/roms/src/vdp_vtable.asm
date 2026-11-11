; Owned V counter table fixture (MIT). It reads the V counter ($7E) once per
; line for one whole frame and reports closed-form summaries of what it saw
; through the RAM mailbox; it judges nothing. Built for 192, 224 and 240
; lines (LINES_HEIGHT). The sequence of V values a line-by-line reader sees
; is the jump table of SMS-VDP section 4.4; the line is 228 Z80 cycles (684
; master ticks, section 4.1).
;
; Fault prevented: a V counter table error that only a program sampling the
; counter once per line can see (a wrong jump position, a missing or an extra
; line), and a sampling loop that is not exactly one line long, which would
; make the records depend on the loop and not on the VDP.
;
; Flow: the TV standard is told from the length of a frame in 31-cycle loops
; (the routine of ports.asm, copied here with the same instructions); that
; gives the line count N, 313 or 262, or 0 for any other length. The start is
; the frame flag. Interrupts stay off. Then N reads, one every 228 cycles:
;   IN A,($7E) 11 + LD (HL),A 7 + INC HL 6 + DEC DE 6 + LD A,D 4 + OR E 4
;   + LD B,10 7 + DJNZ 9 x 13 + 8 + 12 NOP x 4 + JP NZ 10 = 228
; JP takes 10 cycles taken or not, so the last iteration is as long as the
; others. One more read follows at exactly 228 cycles after the last. One read
; tick per line meets every table entry once per frame, so the records do not
; depend on where in the frame the loop began. The summary code runs after
; the loop and takes any time.
;
; Records (value 16 bits, low byte first):
;   01  N, the number of lines read (313, 262, or 0 for another frame length)
;   02  the sum of the samples, modulo 65,536
;   03  the sum of their squares, modulo 65,536
;   04  cyclic descents: a sample above the next, the last against the first
;   05  the largest sample
;   06  the smallest sample
;   07  1 when the extra read equals the first (the loop is one line long);
;       a loop one cycle off drifts by a line over a frame and reads 0
; With N = 0 every record but the first is 0.

.INCLUDE "header.inc"
.IFNDEF LINES_HEIGHT
.DEFINE LINES_HEIGHT 192
.ENDIF
.IF LINES_HEIGHT == 224
.DEFINE MODE_BITS $10
.ELSE
.IF LINES_HEIGHT == 240
.DEFINE MODE_BITS $08
.ELSE
.DEFINE MODE_BITS $00
.ENDIF
.ENDIF

; A frame of 2,048 or more 31-cycle loops is PAL (313 x 228 / 31 = 2,302); an
; NTSC frame is 262 x 228 / 31 = 1,927 (SMS-SYSTEM section 5). Anything under
; NTSC_MIN is neither.
.DEFINE PAL_COUNT 2048
.DEFINE NTSC_MIN 1800
.DEFINE LINES_PAL 313
.DEFINE LINES_NTSC 262
.DEFINE BUF $C400
.DEFINE V_N $C3F0
.DEFINE V_SUM $C3F2
.DEFINE V_SQ $C3F4
.DEFINE V_DESC $C3F6
.DEFINE V_MAX $C3F8
.DEFINE V_MIN $C3F9
.DEFINE V_EXTRA $C3FA
.DEFINE V_DRIFT $C3FB

.BANK 0 SLOT 0
.ORG $0000
    di
    ld sp,$DFF0
    jp main

; A Pause press changes nothing.
.ORG $0066
    retn

main:
    ld a,7
    call mb_init
    ; Mode 4 with the mode-2 bit, the display on, no interrupt enabled; the
    ; height bits are the ones vdp_lines uses.
    ld a,$06
    out ($BF),a
    ld a,$80
    out ($BF),a
    ld a,$40 | MODE_BITS
    out ($BF),a
    ld a,$81
    out ($BF),a
    ; The frame length in 31-cycle loops: INC HL (6) + IN A,($BF) (11)
    ; + OR A (4) + JP P (10) = 31 cycles, as ports.asm.
    in a,($BF)
tv_sync:
    in a,($BF)
    or a
    jp p,tv_sync
    ld hl,0
tv_count:
    inc hl
    in a,($BF)
    or a
    jp p,tv_count
    ld de,PAL_COUNT
    or a
    sbc hl,de
    jr c,tv_not_pal
    ld de,LINES_PAL
    jr tv_known
tv_not_pal:
    add hl,de
    ld de,NTSC_MIN
    or a
    sbc hl,de
    ld de,0
    jr c,tv_known
    ld de,LINES_NTSC
tv_known:
    ; The summary variables start at zero; N is kept.
    ld (V_N),de
    ld hl,0
    ld (V_SUM),hl
    ld (V_SQ),hl
    ld (V_DESC),hl
    xor a
    ld (V_MAX),a
    ld (V_MIN),a
    ld (V_EXTRA),a
    ld (V_DRIFT),a
    ld a,d
    or e
    jr z,summary
    ld hl,BUF
sample:
    in a,($7E)          ; 11
    ld (hl),a           ; 7
    inc hl              ; 6
    dec de              ; 6
    ld a,d              ; 4
    or e                ; 4
    ld b,10             ; 7
pad:
    djnz pad            ; 9 x 13 + 8 = 125
    .REPT 12
    nop                 ; 12 x 4 = 48
    .ENDR
    jp nz,sample        ; 10, taken or not: the iteration is 228 cycles
    ; Exactly 228 cycles after the last read.
    in a,($7E)
    ld (V_EXTRA),a

summary:
    ld bc,(V_N)
    ld a,b
    or c
    jp z,report
    ; The sum.
    ld hl,BUF
    ld de,0
sum_loop:
    ld a,(hl)
    add a,e
    ld e,a
    ld a,d
    adc a,0
    ld d,a
    inc hl
    dec bc
    ld a,b
    or c
    jr nz,sum_loop
    ld (V_SUM),de
    ; The sum of squares.
    ld hl,BUF
    ld bc,(V_N)
    ld de,0
sq_loop:
    push bc
    push hl
    push de
    ld a,(hl)
    call square
    pop de
    add hl,de
    ex de,hl
    pop hl
    pop bc
    inc hl
    dec bc
    ld a,b
    or c
    jr nz,sq_loop
    ld (V_SQ),de
    ; Maximum, minimum and cyclic descents.
    ld a,$FF
    ld (V_MIN),a
    ld hl,BUF
    ld bc,(V_N)
scan_loop:
    ld d,(hl)
    ld a,(V_MAX)
    cp d
    jr nc,scan_not_max
    ld a,d
    ld (V_MAX),a
scan_not_max:
    ld a,(V_MIN)
    cp d
    jr c,scan_not_min
    jr z,scan_not_min
    ld a,d
    ld (V_MIN),a
scan_not_min:
    push hl
    inc hl
    dec bc
    ld a,b
    or c
    jr nz,scan_next
    ld hl,BUF
scan_next:
    ld a,(hl)
    cp d
    jr nc,scan_no_descent
    ld hl,(V_DESC)
    inc hl
    ld (V_DESC),hl
scan_no_descent:
    pop hl
    inc hl
    ld a,b
    or c
    jr nz,scan_loop
    ; Drift: the extra read against the first sample.
    ld a,(BUF)
    ld hl,V_EXTRA
    cp (hl)
    jr nz,report
    ld a,1
    ld (V_DRIFT),a
report:
    ld hl,(V_N)
    ld a,1
    call mb_record
    ld hl,(V_SUM)
    ld a,2
    call mb_record
    ld hl,(V_SQ)
    ld a,3
    call mb_record
    ld hl,(V_DESC)
    ld a,4
    call mb_record
    ld a,(V_MAX)
    ld l,a
    ld h,0
    ld a,5
    call mb_record
    ld a,(V_MIN)
    ld l,a
    ld h,0
    ld a,6
    call mb_record
    ld a,(V_DRIFT)
    ld l,a
    ld h,0
    ld a,7
    call mb_record
    call mb_done
done:
    jr done

; A = x. HL = x * x, shift and add from the top bit. Changes BC, DE, HL.
square:
    ld c,a
    ld e,a
    ld d,0
    ld hl,0
    ld b,8
square_bit:
    add hl,hl
    rlc c
    jr nc,square_skip
    add hl,de
square_skip:
    djnz square_bit
    ret

.INCLUDE "mailbox.inc"
