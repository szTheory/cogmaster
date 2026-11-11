; The Light Phaser fixture: a 32 KiB image that polls both TH lines the way a
; gun title does and reports what it sees through the RAM mailbox; it measures
; and never judges. Plain assembly written from SMS Power's Light Phaser page
; and SMS-CARTRIDGES-AND-PERIPHERALS section 3.3, with PITFALLS section 9 for
; why an aim that is not a dot of the picture is a state of its own.
;
; The gun holds TH low while the dots in front of its lens are bright, TH is
; read on $DD b6 (port 1) and b7 (port 2), and the rise of TH when the light
; ends latches the H counter, which $7F then returns: that is how a program
; learns where on the line the gun points. The sensor answers to light whether
; or not the trigger is pulled, and a pulse is shorter than any loop that
; polls for it, so a console keeps a bit that says TH went low since the last
; read. The program polls both TH bits, so one image serves either port.
;
; The picture never changes after set-up: black, a 16 by 16 white square, a
; 16 by 16 dark grey square (1,1,1, too dim for the sensor) and a one dot tall
; white line across the whole width. The aim in the run's script follows the
; frames, one measurement a frame; the .DEFINEs below name the square, the
; line and the first frame of each part, and the cmake oracle reads them.
; Every position is a multiple of 8 (a tile boundary). A dot is 0-based here;
; an aim in a script is the dot plus one.
;
; The sensor looks at the dots within 6 of the aim. Aimed at the square's
; right edge (dot SQ_X + 15, row SQ_Y + 8) it sees the edge of the square on
; each of its 13 rows, so the light ends at the same dot, SQ_X + 16, on every
; row and the latched H does not depend on the row. Aimed at the line it sees
; one row, and the light ends at the end of the sensor's reach.
;
; Records, value low byte first:
;   01     TH low reads in 256 polls with the aim parked
;   02-0B  $7F read after TH has returned high, at poll periods of 29, 31, 47,
;          64, 97, 100, 101, 200, 114 and 228 Z80 cycles, aim on the square
;   0C-15  $7F read straight after the poll that saw TH low, same periods
;   16     TH low reads in 256 polls with the aim on the dark square
;   17     TH low reads in 256 polls with the aim on the square, trigger never
;          pulled
;   18-19  the V counter read straight after the poll, periods 29 and 64
;   1A     which TH bit saw light in that run, $40 (port 1) or $80 (port 2)
;   1B-1C  the $7F reads after TH returned high and straight after the poll,
;          period 31, aim on the line
; A part that finds no light records $FFFF.
;
; Periods. A sample is the time from one IN to the next. The 31 cycle sample
; is IN 11 + CPL 4 + AND n 7 + RET NZ 5 + NOP 4, and the 29 cycle one swaps
; the IN for IN A,(C) 12 and the AND for AND D 4. RET NZ leaves the routine
; when TH reads low; a routine that finds nothing returns with Z set. The other
; eight periods are a loop: IN 11 + CPL 4 + AND D 4 + RET NZ 5 + a pad +
; DJNZ 13 = 37 plus the pad, which is NOPs (4), LD A,0 (7) and INC HL (6).
; Periods 114 and 228 divide the 228 cycle line, so a loop of either reads one
; fixed column of every row and would never see a pulse that is not there at
; that moment: they are in the list on purpose.

.INCLUDE "header.inc"

.DEFINE ROM_ID 14

; The picture, in dots from the top left of the 256 by 192 field.
.DEFINE SQ_X 96
.DEFINE SQ_Y 40
.DEFINE DARK_X 96
.DEFINE DARK_Y 104
.DEFINE BAND_X 128
.DEFINE BAND_Y 152

; The frame of the script each part is measured in, counted from 1 at power
; on: the set-up ends before the second frame flag and waits for two, so the
; first part runs in frame 4. The ten periods take frames 5 to 14.
.DEFINE PHASE_PARKED 4
.DEFINE PHASE_PERIODS 5
.DEFINE PHASE_DARK 15
.DEFINE PHASE_SQUARE 16
.DEFINE PHASE_BAND 17

; $3F with both TH lines and both TR lines inputs: the levels are b4-b7.
.DEFINE IO_INPUTS $FF
.DEFINE TH_MASK $C0

; Work RAM variables, clear of the mailbox, the stack and the mapper.
.DEFINE VALUE_A $C200
.DEFINE VALUE_B $C202
.DEFINE V_POLL $C204

; Records the 16 bit word at ADDR under the test id.
.MACRO RECORD_WORD ARGS id addr
    ld hl,(\2)
    ld a,\1
    call mb_record
.ENDM

; One measurement in its own frame: wait for the frame flag, wait for the V
; counter to reach the row two above the first row the sensor reaches, run the
; routine, then record the two readings.
.MACRO MEASURE ARGS sweep start id_b id_a
    call wait_frame
    ld e,\2
    call sync_v
    ld c,$DD
    ld d,TH_MASK
    call \1
    call take
    RECORD_WORD \3 VALUE_B
    RECORD_WORD \4 VALUE_A
.ENDM

; A loop of 256 polls whose period is 37 plus the pad of NOPs, LD A,0 and
; INC HL it is given.
.MACRO SWEEP_LOOP ARGS nops sevens sixes
    ld b,0
sweep\@:
    in a,($DD)
    cpl
    and d
    ret nz
    .REPEAT \1
    nop
    .ENDR
    .REPEAT \2
    ld a,0
    .ENDR
    .REPEAT \3
    inc hl
    .ENDR
    djnz sweep\@
    ret
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
    call video
    ld a,IO_INPUTS
    out ($3F),a
    ; The flag of the frame the set-up ended in may or may not be raised, so
    ; two waits end at the start of the third frame whichever it was.
    call wait_frame
    call wait_frame

    ; The aim is parked: no light.
    call wait_frame
    ld e,SQ_Y
    call sync_v
    call count_polls
    ld l,d
    ld h,0
    ld a,$01
    call mb_record

    ; The ten periods, aim on the square.
    MEASURE sweep_29 SQ_Y $02 $0C
    RECORD_WORD $18 V_POLL
    MEASURE sweep_31 SQ_Y $03 $0D
    MEASURE sweep_47 SQ_Y $04 $0E
    MEASURE sweep_64 SQ_Y $05 $0F
    RECORD_WORD $19 V_POLL
    MEASURE sweep_97 SQ_Y $06 $10
    MEASURE sweep_100 SQ_Y $07 $11
    MEASURE sweep_101 SQ_Y $08 $12
    MEASURE sweep_200 SQ_Y $09 $13
    MEASURE sweep_114 SQ_Y $0A $14
    MEASURE sweep_228 SQ_Y $0B $15

    ; The aim is on the dark square: no light.
    call wait_frame
    ld e,DARK_Y
    call sync_v
    call count_polls
    ld l,d
    ld h,0
    ld a,$16
    call mb_record

    ; The aim is on the square and the trigger is never pulled: light.
    call wait_frame
    ld e,SQ_Y
    call sync_v
    call count_polls
    ld l,d
    ld h,0
    ld a,$17
    push de
    call mb_record
    pop de
    ld l,e
    ld h,0
    ld a,$1A
    call mb_record

    ; The aim is on the line.
    MEASURE sweep_31 BAND_Y-2 $1B $1C
    call mb_done
idle:
    call wait_frame
    jr idle

; Display off, the picture loaded, display on: palette entry 1 white, 2 dark
; grey, everything else black.
video:
    ld a,$04
    ld b,$80
    call vdp_reg
    ld a,$FF
    ld b,$82
    call vdp_reg
    ld a,$7F
    ld b,$85
    call vdp_reg
    xor a
    ld b,$87
    call vdp_reg
    xor a
    ld b,$88
    call vdp_reg
    xor a
    ld b,$89
    call vdp_reg
    ; Palette: 32 entries.
    xor a
    out ($BF),a
    ld a,$C0
    out ($BF),a
    ld hl,palette
    ld b,32
    ld c,$BE
    otir
    ; Four tiles at $0000.
    xor a
    out ($BF),a
    ld a,$40
    out ($BF),a
    ld hl,tiles
    ld b,128
    ld c,$BE
    otir
    ; The name table, 1792 bytes of tile 0 (seven runs of 256).
    xor a
    out ($BF),a
    ld a,$78
    out ($BF),a
    ld c,7
clear_names:
    ld b,0
clear_run:
    out ($BE),a
    djnz clear_run
    dec c
    jr nz,clear_names
    ; The sprite table ends at once.
    xor a
    out ($BF),a
    ld a,$7F
    out ($BF),a
    ld a,$D0
    out ($BE),a
    ; The white square, the dark square and the line: name table rows of 64
    ; bytes, columns of 2.
    ld de,$7800 + (SQ_Y / 8) * 64 + (SQ_X / 8) * 2
    ld a,1
    ld b,2
    call put_names
    ld de,$7800 + (SQ_Y / 8 + 1) * 64 + (SQ_X / 8) * 2
    ld a,1
    ld b,2
    call put_names
    ld de,$7800 + (DARK_Y / 8) * 64 + (DARK_X / 8) * 2
    ld a,2
    ld b,2
    call put_names
    ld de,$7800 + (DARK_Y / 8 + 1) * 64 + (DARK_X / 8) * 2
    ld a,2
    ld b,2
    call put_names
    ld de,$7800 + (BAND_Y / 8) * 64
    ld a,3
    ld b,32
    call put_names
    ld a,$40
    ld b,$81
    jp vdp_reg

; A is the value, B the register with bit 7 set.
vdp_reg:
    out ($BF),a
    ld a,b
    out ($BF),a
    ret

; DE is the VRAM address of a name table entry with bit 14 set, A the tile
; and B the number of entries.
put_names:
    ld c,a
    ld a,e
    out ($BF),a
    ld a,d
    out ($BF),a
put_names_loop:
    ld a,c
    out ($BE),a
    xor a
    out ($BE),a
    djnz put_names_loop
    ret

; Waits for the V counter to read E.
sync_v:
    in a,($7E)
    cp e
    jr nz,sync_v
    ret

; 256 polls of TH, about 42 cycles apart. D is the number that read low and E
; the TH bits that did.
count_polls:
    ld d,0
    ld e,0
    ld b,0
count_loop:
    in a,($DD)
    cpl
    and TH_MASK
    jr z,count_next
    or e
    ld e,a
    inc d
count_next:
    djnz count_loop
    ret

; After a routine that polled: Z means it found no light. Otherwise V and $7F
; are read at once, TH is waited out and $7F is read again.
take:
    jr z,take_none
    in a,($7E)
    ld l,a
    ld h,0
    ld (V_POLL),hl
    in a,($7F)
    ld l,a
    ld (VALUE_A),hl
    ld b,0
take_wait:
    in a,($DD)
    cpl
    and TH_MASK
    jr z,take_high
    djnz take_wait
take_high:
    in a,($7F)
    ld l,a
    ld h,0
    ld (VALUE_B),hl
    ret
take_none:
    ld hl,$FFFF
    ld (V_POLL),hl
    ld (VALUE_A),hl
    ld (VALUE_B),hl
    ret

; 29 cycles a sample, 256 samples, C is $DD and D is the TH mask.
sweep_29:
    .REPEAT 256
    in a,(c)
    cpl
    and d
    ret nz
    nop
    .ENDR
    ret

; 31 cycles a sample.
sweep_31:
    .REPEAT 256
    in a,($DD)
    cpl
    and TH_MASK
    ret nz
    nop
    .ENDR
    ret

; The pads: 47 is 37 + NOP + INC HL; 64 is 37 + 5 NOP + LD A,0; 97 is 37 + 15
; NOP; 100 is 37 + 14 NOP + LD A,0; 101 is 37 + 16 NOP; 200 is 37 + 39 NOP +
; LD A,0; 114 is 37 + 11 LD A,0; 228 is 37 + 46 NOP + LD A,0.
sweep_47:
    SWEEP_LOOP 1 0 1
sweep_64:
    SWEEP_LOOP 5 1 0
sweep_97:
    SWEEP_LOOP 15 0 0
sweep_100:
    SWEEP_LOOP 14 1 0
sweep_101:
    SWEEP_LOOP 16 0 0
sweep_200:
    SWEEP_LOOP 39 1 0
sweep_114:
    SWEEP_LOOP 0 11 0
sweep_228:
    SWEEP_LOOP 46 1 0

; Status bit 7 is the frame interrupt flag; reading the status clears it
; (SMS-VDP section 2.6).
wait_frame:
    in a,($BF)
    or a
    jp p,wait_frame
    ret

; Palette 0 black, 1 white, 2 dark grey (1,1,1), the rest black.
palette:
    .DB $00,$3F,$15
    .DSB 29,$00

; Tile 0 is blank, tile 1 solid colour 1, tile 2 solid colour 2, tile 3 has
; colour 1 on its top pixel row only.
tiles:
    .DSB 32,$00
    .REPEAT 8
    .DB $FF,$00,$00,$00
    .ENDR
    .REPEAT 8
    .DB $00,$FF,$00,$00
    .ENDR
    .DB $FF,$00,$00,$00
    .DSB 28,$00

.INCLUDE "mailbox.inc"
