; The Paddle fixture: a 32 KiB image that polls the paddle on port 1 the way a
; game does and reports what it sees through the RAM mailbox; it judges
; nothing. Plain assembly written from the protocol in
; SMS-CARTRIDGES-AND-PERIPHERALS section 3.6.
;
; The Paddle drives D0-D3 with a nibble of its position and says which one on
; TR, a line it clocks itself: TR high is the high nibble, TR low the low one.
; TL is the button, 0 = pressed. The Paddle was made for the Japanese console,
; so the program first runs the region test of the titles (TH of both ports as
; outputs, read back from $DD): the Japanese console reads the outputs low and
; the export console reads them back as written (SMS-SYSTEM section 5). On the
; export console it records that and stops, which is what a title does when it
; finds no Paddle to read.
;
; Records, value low byte first:
;   01     the region test: 1 on the Japanese console, 0 on the export one
;   02-09  ones read on TR in 256 samples with a sample period of 29, 31, 47,
;          64, 97, 100, 101 and 200 Z80 cycles
;   0A-0B  the same at 199 and 398 cycles, the lengths a clock of this
;          frequency nearly locks to
;   0C-19  frames 1 to 14: the position as the reader assembles it from the
;          two nibbles in the low byte, 1 in the high byte when TL reads
;          pressed, and $FFFF if TR never toggles
;
; A sample period is the time from one IN to the next. The detect loop is
; IN, BIT, JR, INC, DJNZ: 11 + 8 + 7 + 4 + 13 = 43 cycles when TR reads 1 and
; one more, 44, when it reads 0 (the JR is taken, 12 cycles, and skips the
; INC). The period named below is the 43-cycle path; a padded loop adds the
; same cycles to both paths. A count of ones is kept in one byte, so a TR
; stuck high counts 256 and wraps to 0, as does a TR stuck low.

.INCLUDE "header.inc"

.DEFINE ROM_ID 12
; $3F with both TH lines outputs and both TR lines inputs: the TH levels are
; b5 (port 1) and b7 (port 2).
.DEFINE TH_HIGH $F5
.DEFINE TH_LOW $55
; The reader gives up on a TR that does not change after this many polls, a
; little under 3,000 cycles, well past the 100 cycles a half period takes.
.DEFINE POLL_LIMIT 64
.DEFINE FRAME_NO $C210
.DEFINE REGION $C211

; Padding for a looped sweep: NOPS of 4 cycles and SEVENS of 7 (LD A,0).
.MACRO SWEEP_LOOP ARGS nops sevens
    ld d,0
    ld b,0
sweep\@:
    in a,($DC)
    bit 5,a
    jr z,+
    inc d
+:
    .REPEAT \1
    nop
    .ENDR
    .REPEAT \2
    ld a,0
    .ENDR
    djnz sweep\@
.ENDM

; Records the count in D under the test id.
.MACRO RECORD_COUNT ARGS id
    ld l,d
    ld h,0
    ld a,\1
    call mb_record
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

    ; The region test, as in the ports fixture: outputs high, then low.
    ld a,TH_HIGH
    out ($3F),a
    in a,($DD)
    and $C0
    ld l,a
    ld a,TH_LOW
    out ($3F),a
    in a,($DD)
    and $C0
    or l
    ld l,a
    ld a,TH_HIGH
    out ($3F),a
    ld a,l
    or a
    ld hl,0
    jr nz,region_done
    inc l
region_done:
    ld a,l
    ld (REGION),a
    ld a,$01
    call mb_record
    ld a,(REGION)
    or a
    jr nz,japan
    call mb_done
export_idle:
    jr export_idle

    ; Frames 1 to 14: the first read is made in frame 1 and each later one
    ; after the frame flag, so read n sees the motion of calls 1 to n.
japan:
    ld a,1
    ld (FRAME_NO),a
position_loop:
    call read_paddle
    call show
    ld a,(FRAME_NO)
    add a,$0B
    call mb_record
    ld a,(FRAME_NO)
    inc a
    ld (FRAME_NO),a
    call wait_frame
    ld a,(FRAME_NO)
    cp 15
    jr nz,position_loop

    ; The sweep: eight lengths the criterion judges, then the two near-lock
    ; lengths. Each pad is counted: 47 is 43 + 1 NOP; 64 is 43 + 3 x 7;
    ; 97 is 43 + 10 NOP + 2 x 7; 100 is 43 + 9 NOP + 3 x 7; 101 is
    ; 43 + 11 NOP + 2 x 7; 200 is 43 + 34 NOP + 3 x 7; 199 is 43 + 39 NOP;
    ; 398 is 43 + 87 NOP + 7.
    call sweep_29
    RECORD_COUNT $02
    call sweep_31
    RECORD_COUNT $03
    SWEEP_LOOP 1 0
    RECORD_COUNT $04
    SWEEP_LOOP 0 3
    RECORD_COUNT $05
    SWEEP_LOOP 10 2
    RECORD_COUNT $06
    SWEEP_LOOP 9 3
    RECORD_COUNT $07
    SWEEP_LOOP 11 2
    RECORD_COUNT $08
    SWEEP_LOOP 34 3
    RECORD_COUNT $09
    SWEEP_LOOP 39 0
    RECORD_COUNT $0A
    SWEEP_LOOP 87 1
    RECORD_COUNT $0B
    call mb_done

    ; Every later frame reads and shows the position, so the picture follows
    ; the paddle for as long as the run lasts.
poll_loop:
    call read_paddle
    call show
    call wait_frame
    jr poll_loop

; One reading of the position: wait for TR low and keep that read (the low
; nibble and the button), wait for TR high and take the high nibble, as a
; title does. HL is the position in L and 1 in H when TL reads pressed, or
; $FFFF when TR does not change within the bound.
read_paddle:
    ld b,POLL_LIMIT
read_low:
    in a,($DC)
    bit 5,a
    jr z,read_have_low
    djnz read_low
    jr read_fail
read_have_low:
    ld c,a
    ld b,POLL_LIMIT
read_high:
    in a,($DC)
    bit 5,a
    jr nz,read_have_high
    djnz read_high
    jr read_fail
read_have_high:
    and $0F
    add a,a
    add a,a
    add a,a
    add a,a
    ld l,a
    ld a,c
    and $0F
    or l
    ld l,a
    ld a,c
    cpl
    and $10
    rrca
    rrca
    rrca
    rrca
    ld h,a
    ret
read_fail:
    ld hl,$FFFF
    ret

; 29 cycles a sample, 256 samples, no loop: IN 11 + AND 7 + JR 7 + INC 4 when
; TR reads 1, one more when it reads 0. The result is in D.
sweep_29:
    ld d,0
    .REPEAT 256
    in a,($DC)
    and $20
    jr z,+
    inc d
+:
    .ENDR
    ret

; 31 cycles a sample: IN A,(C) 12 + BIT 8 + JR 7 + INC 4, and one more on a 0.
sweep_31:
    ld d,0
    ld c,$DC
    .REPEAT 256
    in a,(c)
    bit 5,a
    jr z,+
    inc d
+:
    .ENDR
    ret

; The backdrop colour (palette entry 16, the display is off): the low six
; bits of the position, so a change of one count changes the picture. A
; failed reading shows as it is.
show:
    ld a,l
    and $3F
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

.INCLUDE "mailbox.inc"
