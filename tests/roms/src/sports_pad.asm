; The Sports Pad fixture: a 32 KiB image that reads the pad on port 1 the way
; a game does and reports what it reads through the RAM mailbox; it judges
; nothing. Plain assembly written from the protocol in
; SMS-CARTRIDGES-AND-PERIPHERALS section 3.5.
;
; A read drives TH low and waits 80 microseconds (286 Z80 cycles), reads
; $DC (D0-D3 the X high nibble), drives TH high and waits 40 microseconds
; (143 cycles), reads the X low nibble, and does the same twice more for Y.
; TL and TR are buttons 1 and 2, 0 = pressed. The pad sends the negated
; motion, so a script that moves right by 5 reads $FB.
;
; The program polls once in each frame: the first read is made in frame 1,
; and each later one after the frame flag, which the core raises at the cut
; between two frames. It shows the bytes it decoded as the backdrop colour.
;
; Records, value low byte first:
;   01-0A  frames 1 to 10: the X byte, then the Y byte
;   0B     frame 11: the X high nibble after the first TH fall, then the
;          nibble after a second fall 511 cycles after the TH rise
;   0C     frame 12: the same with 512 cycles
;   0D-0F  frames 13 to 15: the buttons (b0 button 1, b1 button 2) as the
;          first read saw them, then as the last read saw them
;
; The pad starts a new read when TH has been high for 512 cycles: the core
; counts 3 ticks to a Z80 cycle, so that is SPORTS_IDLE_TICKS = 512 x 3
; ticks, and a gap of 511 cycles is 1,533 ticks, the nearest whole-cycle gap
; below it. Two TH writes are 11 cycles apart plus the instructions
; between them, so the gaps below are counted from write to write.

.INCLUDE "header.inc"

.DEFINE ROM_ID 11
; $3F with both TH lines outputs and both TR lines inputs: the TH levels are
; b5 (port 1) and b7 (port 2); TH of port 2 stays high.
.DEFINE TH_HIGH $F5
.DEFINE TH_LOW $D5
; Mailbox side variables, clear of the mailbox, the stack and the mapper.
.DEFINE NIBBLES $C200
.DEFINE X_BYTE $C204
.DEFINE Y_BYTE $C205
.DEFINE BUTTONS_FIRST $C206
.DEFINE BUTTONS_LAST $C207
.DEFINE FRAME_NO $C210
.DEFINE SCRATCH $C21F

; 143 cycles: LD B 7, the CALL 17, DJNZ 13 x 6 + 8, RET 10, then 23.
.MACRO WAIT_143
    ld b,7
    call delay_b
    ld a,0
    nop
    nop
    nop
    nop
.ENDM

.MACRO WAIT_286
    WAIT_143
    WAIT_143
.ENDM

.BANK 0 SLOT 0
.ORG $0000
    di
    ld sp,$DFF0
    jp main

; A Pause press changes nothing.
.ORG $0066
    retn

main:
    ld a,ROM_ID
    call mb_init
    ld a,TH_HIGH
    out ($3F),a
    in a,($BF)

    ; Frames 1 to 10: one record each of the X and Y bytes.
    ld a,1
    ld (FRAME_NO),a
motion_loop:
    call read_pad
    call show
    ld a,(X_BYTE)
    ld l,a
    ld a,(Y_BYTE)
    ld h,a
    ld a,(FRAME_NO)
    call mb_record
    ld a,(FRAME_NO)
    inc a
    ld (FRAME_NO),a
    call wait_frame
    ld a,(FRAME_NO)
    cp 11
    jr nz,motion_loop

    ; Frame 11: abort after the X high nibble, then pause TH high for 511
    ; cycles: write to write is 11 + 493 + 7 = 511, one cycle short of the
    ; pad's limit, so the read goes on with the Y high nibble.
    ld a,TH_LOW
    out ($3F),a
    WAIT_286
    in a,($DC)
    and $0F
    ld (NIBBLES),a
    ld a,TH_HIGH
    out ($3F),a
    ld b,34
    call delay_b
    ld a,0
    ld a,0
    nop
    nop
    ld a,TH_LOW
    out ($3F),a
    WAIT_143
    in a,($DC)
    and $0F
    ld (NIBBLES+1),a
    ld a,TH_HIGH
    out ($3F),a
    ld hl,(NIBBLES)
    ld a,$0B
    call mb_record
    call wait_frame

    ; Frame 12: the same with 512 cycles, the pad's limit: the pad starts
    ; again with the X high nibble of what motion is left, which is none.
    ld a,TH_LOW
    out ($3F),a
    WAIT_286
    in a,($DC)
    and $0F
    ld (NIBBLES),a
    ld a,TH_HIGH
    out ($3F),a
    ld b,34
    call delay_b
    ld a,0
    nop
    nop
    nop
    nop
    ld a,TH_LOW
    out ($3F),a
    WAIT_143
    in a,($DC)
    and $0F
    ld (NIBBLES+1),a
    ld a,TH_HIGH
    out ($3F),a
    ld hl,(NIBBLES)
    ld a,$0C
    call mb_record
    call wait_frame

    ; Frames 13 to 15: the buttons, with the pad released between them.
    ld a,13
    ld (FRAME_NO),a
button_loop:
    call read_pad
    call show
    ld a,(BUTTONS_FIRST)
    ld l,a
    ld a,(BUTTONS_LAST)
    ld h,a
    ld a,(FRAME_NO)
    call mb_record
    ld a,(FRAME_NO)
    inc a
    ld (FRAME_NO),a
    call wait_frame
    ld a,(FRAME_NO)
    cp 16
    jr nz,button_loop
    call mb_done

    ; Every later frame polls and shows the bytes, so the picture follows
    ; the pad for as long as the run lasts.
poll_loop:
    call read_pad
    call show
    call wait_frame
    jr poll_loop

; One whole read; X in X_BYTE, Y in Y_BYTE, the buttons of the first and the
; last nibble read in BUTTONS_FIRST and BUTTONS_LAST.
read_pad:
    ld a,TH_LOW
    out ($3F),a
    WAIT_286
    in a,($DC)
    ld (NIBBLES),a
    ld a,TH_HIGH
    out ($3F),a
    WAIT_143
    in a,($DC)
    ld (NIBBLES+1),a
    ld a,TH_LOW
    out ($3F),a
    WAIT_143
    in a,($DC)
    ld (NIBBLES+2),a
    ld a,TH_HIGH
    out ($3F),a
    WAIT_143
    in a,($DC)
    ld (NIBBLES+3),a
    ld a,(NIBBLES)
    call high_nibble
    ld b,a
    ld a,(NIBBLES+1)
    and $0F
    or b
    ld (X_BYTE),a
    ld a,(NIBBLES+2)
    call high_nibble
    ld b,a
    ld a,(NIBBLES+3)
    and $0F
    or b
    ld (Y_BYTE),a
    ld a,(NIBBLES)
    call button_bits
    ld (BUTTONS_FIRST),a
    ld a,(NIBBLES+3)
    call button_bits
    ld (BUTTONS_LAST),a
    ret

; A = a $DC reading; returns D0-D3 in b7-b4.
high_nibble:
    and $0F
    add a,a
    add a,a
    add a,a
    add a,a
    ret

; A = a $DC reading; returns TL in b0 and TR in b1, 1 = pressed.
button_bits:
    cpl
    rrca
    rrca
    rrca
    rrca
    and 3
    ret

; The backdrop colour (palette entry 16, the display is off): the low three
; bits of X for red and the low three bits of Y for green.
show:
    ld a,(Y_BYTE)
    and 7
    add a,a
    add a,a
    add a,a
    ld b,a
    ld a,(X_BYTE)
    and 7
    or b
    ld b,a
    ld a,16
    out ($BF),a
    ld a,$C0
    out ($BF),a
    ld a,b
    out ($BE),a
    ret

; Status bit 7 is the frame interrupt flag; reading the status clears it
; (SMS-VDP section 2.6).
wait_frame:
    in a,($BF)
    or a
    jp p,wait_frame
    ret

; B = n loops: CALL 17 + DJNZ 13 x (n - 1) + 8 + RET 10 = 13 n + 22 cycles.
delay_b:
    djnz delay_b
    ret

.INCLUDE "mailbox.inc"
