; The PSG coverage fixture: a 32 KiB image whose Z80 plays a table of Sega PSG
; writes, one frame at a time, and reports counters through the RAM mailbox;
; it judges nothing. The sound is judged by the runner's audio hashes and the
; counters by tests/diag/expected.csv. The same source assembled with
; FIXTURE_GG adds writes to the Game Gear stereo port $06 and reports the
; same records.
;
; Coverage is every register bit set and cleared plus the extremes, not every
; period (SMS-AUDIO section 2): the latch and data protocol, periods 0, 1, 2,
; 3, $0FE, $1FF and $3FF, a single bit walked through all ten period bits,
; every attenuation on every channel, every noise control with tone 2 at
; three periods under the tone 2 rate, the noise register resetting on a
; data byte, and a frame of attenuation writes 29 cycles apart.
;
; The table is a byte stream the player reads, with no input and no interrupt:
;   OP_WAIT n   wait n frames (the VDP frame flag, SMS-VDP section 2.6)
;   OP_PSG b    write b to the PSG port $7F and count the write
;   OP_GG b     write b to the Game Gear stereo port $06 (FIXTURE_GG only)
;   OP_MARK     close a segment with two records
;   OP_POPS     2,048 attenuation writes 29 cycles apart, counted as 2,048
;   OP_END      report done
; The write counter counts PSG port writes only, never a $06 write, and a
; $06 write adds no frame wait, record or count, so the two images report the
; same values.
;
; Records, value low byte first, for segment s of 19: id 2s-1 is the frames
; waited so far and id 2s is the PSG writes made so far, both when the
; segment closes. The mailbox holds 60 records; 38 are used.
;
; Tone 0 sounds at attenuation 0 from frame 20 to frame 70. No two channels
; sound at attenuation 0 at once.

.INCLUDE "header.inc"

.DEFINE ROM_ID 8
.DEFINE FRAMES $C210
.DEFINE WRITES $C212
.DEFINE SEGMENT $C214

.DEFINE OP_END 0
.DEFINE OP_WAIT 1
.DEFINE OP_PSG 2
.DEFINE OP_GG 3
.DEFINE OP_MARK 4
.DEFINE OP_POPS 5

; Counts the segments so the assembler can check the mailbox has room.
.DEFINE SEGMENTS 0

; One latch byte, then one data byte: channel \1 tone period \2 (10 bits).
.MACRO S_TONE
    .DB OP_PSG, $80 | (\1 << 5) | (\2 & $0F), OP_PSG, (\2 >> 4) & $3F
.ENDM

; Latch channel \1 volume, attenuation \2.
.MACRO S_VOL
    .DB OP_PSG, $90 | (\1 << 5) | \2
.ENDM

; Latch the noise register, control \1.
.MACRO S_NOISE
    .DB OP_PSG, $E0 | \1
.ENDM

; Any byte \1 to the PSG port.
.MACRO S_PSG
    .DB OP_PSG, \1
.ENDM

.MACRO S_WAIT
    .DB OP_WAIT, \1
.ENDM

; A byte \1 to port $06, present in the Game Gear image only.
.MACRO S_GG
.IFDEF FIXTURE_GG
    .DB OP_GG, \1
.ENDIF
.ENDM

.MACRO S_POPS
    .DB OP_POPS
.ENDM

.MACRO S_MARK
    .DB OP_MARK
.REDEFINE SEGMENTS SEGMENTS+1
.ENDM

.BANK 0 SLOT 0
.ORG 0
    di
    ld sp,$DFF0
    jp main
.ORG $66
    retn
main:
    ld a,ROM_ID
    call mb_init
    ld hl,0
    ld (FRAMES),hl
    ld (WRITES),hl
    ld a,1
    ld (SEGMENT),a
    ; Clear a frame flag the VDP already raised, then count frames from here.
    in a,($BF)
    ld ix,table

next:
    ld a,(ix+0)
    inc ix
    or a
    jr z,finish
    dec a
    jr z,op_wait
    dec a
    jr z,op_psg
    dec a
    jr z,op_gg
    dec a
    jr z,op_mark
    dec a
    jr z,op_pops
    jr finish

op_wait:
    ld b,(ix+0)
    inc ix
wait_loop:
    call wait_frame
    ld hl,(FRAMES)
    inc hl
    ld (FRAMES),hl
    djnz wait_loop
    jr next

op_psg:
    ld a,(ix+0)
    inc ix
    out ($7F),a
    ld hl,(WRITES)
    inc hl
    ld (WRITES),hl
    jr next

op_gg:
.IFDEF FIXTURE_GG
    ld a,(ix+0)
    inc ix
    out ($06),a
.ENDIF
    jr next

op_mark:
    ld a,(SEGMENT)
    add a,a
    dec a
    ld hl,(FRAMES)
    call mb_record
    ld a,(SEGMENT)
    add a,a
    ld hl,(WRITES)
    call mb_record
    ld hl,SEGMENT
    inc (hl)
    jr next

; Alternate attenuation 0 and 15 on channel 0, 256 writes at a time. Each
; write is OUT (C),A, 12 cycles, then EX AF,AF', 4, then DJNZ taken, 13:
; 29 cycles. 8 x 256 writes are about one frame of 59,736 cycles.
op_pops:
    ld c,$7F
    ld a,$90
    ex af,af'
    ld a,$9F
    ex af,af'
    ld d,8
pops_outer:
    ld b,0
pops_loop:
    out (c),a
    ex af,af'
    djnz pops_loop
    dec d
    jr nz,pops_outer
    ld hl,(WRITES)
    ld de,2048
    add hl,de
    ld (WRITES),hl
    jr next

finish:
    call mb_done
done:
    jr done

; Reading the status clears the frame flag (SMS-VDP section 2.6).
wait_frame:
    in a,($BF)
    or a
    jp p,wait_frame
    ret

table:
; Segment 1: boot silence: every attenuation $F.
    S_VOL 0, 15
    S_VOL 1, 15
    S_VOL 2, 15
    S_VOL 3, 15
    S_WAIT 20
    S_MARK

; Segment 2: the loud constant tone: tone 0 at 440 Hz, attenuation 0, frames 20 to 70, no input.
    S_TONE 0, $0FE
    S_VOL 0, 0
    S_WAIT 50
    S_VOL 0, 15
    S_MARK

; Segment 3: protocol: a latch byte then a data byte on a tone.
    S_VOL 1, 8
    S_PSG $A5 ; latch tone 1, low nibble 5
    S_WAIT 2
    S_PSG $0F ; data: high six bits, period $0F5
    S_WAIT 3
    S_VOL 1, 15
    S_MARK

; Segment 4: protocol: a data byte after a volume or noise latch rewrites that register.
    S_PSG $9A ; latch tone 0 volume, attenuation $A
    S_PSG $03 ; data: the volume becomes 3
    S_WAIT 3
    S_PSG $E5 ; latch the noise register, white, rate 1
    S_PSG $06 ; data: the control becomes 6
    S_WAIT 3
    S_VOL 0, 15
    S_VOL 3, 15
    S_MARK

; Segment 5: protocol: the latch survives data bytes.
    S_VOL 2, 8
    S_PSG $C3 ; latch tone 2, low nibble 3
    S_PSG $01
    S_WAIT 2
    S_PSG $02
    S_WAIT 2
    S_PSG $3F
    S_WAIT 3
    S_VOL 2, 15
    S_MARK

; Segment 6: periods 0, 1, 2, 3, $0FE, $1FF and $3FF on tone 0.
    S_VOL 0, 6
    S_TONE 0, $000
    S_WAIT 2
    S_TONE 0, $001
    S_WAIT 2
    S_TONE 0, $002
    S_WAIT 2
    S_TONE 0, $003
    S_WAIT 2
    S_TONE 0, $0FE
    S_WAIT 2
    S_TONE 0, $1FF
    S_WAIT 2
    S_TONE 0, $3FF
    S_WAIT 2
    S_VOL 0, 15
    S_MARK

; Segment 7: periods 0, 1, 2, 3, $0FE, $1FF and $3FF on tone 1.
    S_VOL 1, 6
    S_TONE 1, $000
    S_WAIT 2
    S_TONE 1, $001
    S_WAIT 2
    S_TONE 1, $002
    S_WAIT 2
    S_TONE 1, $003
    S_WAIT 2
    S_TONE 1, $0FE
    S_WAIT 2
    S_TONE 1, $1FF
    S_WAIT 2
    S_TONE 1, $3FF
    S_WAIT 2
    S_VOL 1, 15
    S_MARK

; Segment 8: periods 0, 1, 2, 3, $0FE, $1FF and $3FF on tone 2.
    S_VOL 2, 6
    S_TONE 2, $000
    S_WAIT 2
    S_TONE 2, $001
    S_WAIT 2
    S_TONE 2, $002
    S_WAIT 2
    S_TONE 2, $003
    S_WAIT 2
    S_TONE 2, $0FE
    S_WAIT 2
    S_TONE 2, $1FF
    S_WAIT 2
    S_TONE 2, $3FF
    S_WAIT 2
    S_VOL 2, 15
    S_MARK

; Segment 9: walking ones through all ten period bits on tone 0.
    S_VOL 0, 6
    S_TONE 0, $001
    S_WAIT 2
    S_TONE 0, $002
    S_WAIT 2
    S_TONE 0, $004
    S_WAIT 2
    S_TONE 0, $008
    S_WAIT 2
    S_TONE 0, $010
    S_WAIT 2
    S_TONE 0, $020
    S_WAIT 2
    S_TONE 0, $040
    S_WAIT 2
    S_TONE 0, $080
    S_WAIT 2
    S_TONE 0, $100
    S_WAIT 2
    S_TONE 0, $200
    S_WAIT 2
    S_VOL 0, 15
    S_MARK

; Segment 10: walking ones through all ten period bits on tone 1.
    S_VOL 1, 6
    S_TONE 1, $001
    S_WAIT 2
    S_TONE 1, $002
    S_WAIT 2
    S_TONE 1, $004
    S_WAIT 2
    S_TONE 1, $008
    S_WAIT 2
    S_TONE 1, $010
    S_WAIT 2
    S_TONE 1, $020
    S_WAIT 2
    S_TONE 1, $040
    S_WAIT 2
    S_TONE 1, $080
    S_WAIT 2
    S_TONE 1, $100
    S_WAIT 2
    S_TONE 1, $200
    S_WAIT 2
    S_VOL 1, 15
    S_MARK

; Segment 11: walking ones through all ten period bits on tone 2.
    S_VOL 2, 6
    S_TONE 2, $001
    S_WAIT 2
    S_TONE 2, $002
    S_WAIT 2
    S_TONE 2, $004
    S_WAIT 2
    S_TONE 2, $008
    S_WAIT 2
    S_TONE 2, $010
    S_WAIT 2
    S_TONE 2, $020
    S_WAIT 2
    S_TONE 2, $040
    S_WAIT 2
    S_TONE 2, $080
    S_WAIT 2
    S_TONE 2, $100
    S_WAIT 2
    S_TONE 2, $200
    S_WAIT 2
    S_VOL 2, 15
    S_MARK

; Segment 12: all sixteen attenuations on channel 0.
    S_TONE 0, $0A0
    S_VOL 0, 0
    S_GG $01
    S_WAIT 1
    S_VOL 0, 1
    S_WAIT 1
    S_VOL 0, 2
    S_WAIT 1
    S_VOL 0, 3
    S_WAIT 1
    S_VOL 0, 4
    S_WAIT 1
    S_VOL 0, 5
    S_GG $02
    S_WAIT 1
    S_VOL 0, 6
    S_WAIT 1
    S_VOL 0, 7
    S_WAIT 1
    S_VOL 0, 8
    S_WAIT 1
    S_VOL 0, 9
    S_WAIT 1
    S_VOL 0, 10
    S_GG $04
    S_WAIT 1
    S_VOL 0, 11
    S_WAIT 1
    S_VOL 0, 12
    S_WAIT 1
    S_VOL 0, 13
    S_WAIT 1
    S_VOL 0, 14
    S_WAIT 1
    S_VOL 0, 15
    S_WAIT 1
    S_VOL 0, 15
    S_MARK

; Segment 13: all sixteen attenuations on channel 1.
    S_TONE 1, $0A0
    S_VOL 1, 0
    S_GG $08
    S_WAIT 1
    S_VOL 1, 1
    S_WAIT 1
    S_VOL 1, 2
    S_WAIT 1
    S_VOL 1, 3
    S_WAIT 1
    S_VOL 1, 4
    S_WAIT 1
    S_VOL 1, 5
    S_GG $10
    S_WAIT 1
    S_VOL 1, 6
    S_WAIT 1
    S_VOL 1, 7
    S_WAIT 1
    S_VOL 1, 8
    S_WAIT 1
    S_VOL 1, 9
    S_WAIT 1
    S_VOL 1, 10
    S_GG $20
    S_WAIT 1
    S_VOL 1, 11
    S_WAIT 1
    S_VOL 1, 12
    S_WAIT 1
    S_VOL 1, 13
    S_WAIT 1
    S_VOL 1, 14
    S_WAIT 1
    S_VOL 1, 15
    S_WAIT 1
    S_VOL 1, 15
    S_MARK

; Segment 14: all sixteen attenuations on channel 2.
    S_TONE 2, $0A0
    S_VOL 2, 0
    S_GG $40
    S_WAIT 1
    S_VOL 2, 1
    S_WAIT 1
    S_VOL 2, 2
    S_WAIT 1
    S_VOL 2, 3
    S_WAIT 1
    S_VOL 2, 4
    S_WAIT 1
    S_VOL 2, 5
    S_GG $80
    S_WAIT 1
    S_VOL 2, 6
    S_WAIT 1
    S_VOL 2, 7
    S_WAIT 1
    S_VOL 2, 8
    S_WAIT 1
    S_VOL 2, 9
    S_WAIT 1
    S_VOL 2, 10
    S_GG $0F
    S_WAIT 1
    S_VOL 2, 11
    S_WAIT 1
    S_VOL 2, 12
    S_WAIT 1
    S_VOL 2, 13
    S_WAIT 1
    S_VOL 2, 14
    S_WAIT 1
    S_VOL 2, 15
    S_WAIT 1
    S_VOL 2, 15
    S_MARK

; Segment 15: all sixteen attenuations on channel 3 (noise, white, rate 1).
    S_NOISE 5
    S_VOL 3, 0
    S_GG $F0
    S_WAIT 1
    S_VOL 3, 1
    S_WAIT 1
    S_VOL 3, 2
    S_WAIT 1
    S_VOL 3, 3
    S_WAIT 1
    S_VOL 3, 4
    S_WAIT 1
    S_VOL 3, 5
    S_GG $00
    S_WAIT 1
    S_VOL 3, 6
    S_WAIT 1
    S_VOL 3, 7
    S_WAIT 1
    S_VOL 3, 8
    S_WAIT 1
    S_VOL 3, 9
    S_WAIT 1
    S_VOL 3, 10
    S_GG $FF
    S_WAIT 1
    S_VOL 3, 11
    S_WAIT 1
    S_VOL 3, 12
    S_WAIT 1
    S_VOL 3, 13
    S_WAIT 1
    S_VOL 3, 14
    S_WAIT 1
    S_VOL 3, 15
    S_WAIT 1
    S_VOL 3, 15
    S_MARK

; Segment 16: the eight noise controls.
    S_TONE 2, $0C0
    S_VOL 3, 4
    S_NOISE 0
    S_WAIT 2
    S_NOISE 1
    S_WAIT 2
    S_NOISE 2
    S_WAIT 2
    S_NOISE 3
    S_WAIT 2
    S_NOISE 4
    S_WAIT 2
    S_NOISE 5
    S_WAIT 2
    S_NOISE 6
    S_WAIT 2
    S_NOISE 7
    S_WAIT 2
    S_VOL 3, 15
    S_MARK

; Segment 17: noise mode 3 with tone 2 at period 0, a mid value and a high value, white then periodic.
    S_VOL 3, 4
    S_NOISE 7
    S_TONE 2, $000
    S_WAIT 3
    S_TONE 2, $100
    S_WAIT 3
    S_TONE 2, $3F0
    S_WAIT 3
    S_NOISE 3
    S_WAIT 3
    S_VOL 3, 15
    S_MARK

; Segment 18: the noise register resets on a data byte.
    S_VOL 3, 4
    S_NOISE 4
    S_WAIT 2
    S_PSG $04 ; data: control 4 again, the register resets
    S_WAIT 2
    S_PSG $05 ; data: control 5
    S_WAIT 2
    S_VOL 3, 15
    S_MARK

; Segment 19: volume pops: attenuation writes 29 cycles apart for about a frame.
    S_TONE 0, $000
    S_VOL 0, 15
    S_POPS
    S_WAIT 1
    S_VOL 0, 15
    S_MARK
    .DB OP_END

.INCLUDE "mailbox.inc"

; Every segment closes with two records, and the mailbox holds MB_RECORDS_MAX.
.IF SEGMENTS != 19
.FAIL "psg_sweep: the table must hold 19 segments, tests/diag/expected.csv has 38 rows"
.ENDIF
.IF SEGMENTS * 2 > MB_RECORDS_MAX
.FAIL "psg_sweep: more records than the mailbox holds"
.ENDIF
