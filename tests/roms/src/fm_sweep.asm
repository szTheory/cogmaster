; The FM unit fixture: a 32 KiB image whose Z80 first detects the YM2413 FM
; unit through port $F2, reports whether it found one, then plays a table of
; FM writes one frame at a time and reports counters through the RAM mailbox;
; it judges nothing. The sound is judged by the runner's audio hashes, the
; counters and the detect flag by tests/diag/expected.csv.
;
; Detection is this file's own routine (the Master System FM unit answers
; port $F2 with the value last written in bits 1-0, SMS-AUDIO section 4.1).
; Every value 0 to 3 is written and read back; the unit is present only when
; all four reads match in bits 1-0, so a console that reads $DC or open bus
; at $F2 fails at the first mismatch. The value read first is written back
; afterwards, so the routine leaves the port as it found it. On a console
; without the unit the same script runs and its writes change nothing.
;
; The table is a byte stream the player reads, with no input and no interrupt:
;   OP_WAIT n     wait n frames (the VDP frame flag, SMS-VDP section 2.6)
;   OP_FM r v     write register r to port $F0 and value v to port $F1, with
;                 the chip's waits after each (12 and 84 clocks, SMS-AUDIO
;                 section 4.1; the chip's clock is the Z80's), and count one
;   OP_F2 v       write v to port $F2 and count one
;   OP_PSG b      write b to the PSG port $7F; not counted
;   OP_MARK       close a segment with two records
;   OP_END        report done
;
; Records, value low byte first: id 1 is the detect flag; for segment s of 3,
; id 2s is the frames waited so far and id 2s+1 is the FM unit writes made so
; far ($F0/$F1 pairs and $F2 writes, each counted once), both when the
; segment closes. The mailbox holds 60 records; 7 are used.
;
; Segment 1 plays instrument 1 as three notes, then a note in another block.
; Segment 2 turns rhythm mode on and keys each drum, then all five together.
; Segment 3 holds an FM note and a PSG tone while $F2 steps through 0, 1, 2
; and 3 (PSG only, FM only, neither, both). The channel volumes and the
; PSG attenuation leave the mixed peak well under the headroom line.

.INCLUDE "header.inc"

.DEFINE ROM_ID 9
.DEFINE FRAMES $C210
.DEFINE WRITES $C212
.DEFINE SEGMENT $C214

.DEFINE OP_END 0
.DEFINE OP_WAIT 1
.DEFINE OP_FM 2
.DEFINE OP_F2 3
.DEFINE OP_PSG 4
.DEFINE OP_MARK 5

.MACRO S_WAIT
    .DB OP_WAIT, \1
.ENDM

.MACRO S_FM
    .DB OP_FM, \1, \2
.ENDM

.MACRO S_F2
    .DB OP_F2, \1
.ENDM

.MACRO S_PSG
    .DB OP_PSG, \1
.ENDM

.MACRO S_MARK
    .DB OP_MARK
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
    call detect_fm
    ld l,a
    ld h,0
    ld a,1
    call mb_record
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
    jr z,op_fm
    dec a
    jr z,op_f2
    dec a
    jr z,op_psg
    dec a
    jr z,op_mark
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

op_fm:
    ld a,(ix+0)
    out ($F0),a
    call delay_address
    ld a,(ix+1)
    out ($F1),a
    inc ix
    inc ix
    call delay_data
    call count_write
    jr next

op_f2:
    ld a,(ix+0)
    inc ix
    out ($F2),a
    call count_write
    jr next

op_psg:
    ld a,(ix+0)
    inc ix
    out ($7F),a
    jr next

op_mark:
    ld a,(SEGMENT)
    add a,a
    ld hl,(FRAMES)
    call mb_record
    ld a,(SEGMENT)
    add a,a
    inc a
    ld hl,(WRITES)
    call mb_record
    ld hl,SEGMENT
    inc (hl)
    jr next

finish:
    call mb_done
done:
    jr done

count_write:
    ld hl,(WRITES)
    inc hl
    ld (WRITES),hl
    ret

; Reading the status clears the frame flag (SMS-VDP section 2.6).
wait_frame:
    in a,($BF)
    or a
    jp p,wait_frame
    ret

; At least 12 Z80 cycles: the call and the return alone take 27.
delay_address:
    ret

; At least 84 Z80 cycles: the call, the return and six loop passes take 109.
delay_data:
    ld b,6
delay_data_loop:
    djnz delay_data_loop
    ret

; Returns A = 1 when port $F2 holds each of the values 0 to 3 written to it
; in bits 1-0, else A = 0. Bits 7 to 5 of the unit's read are a counter and
; are never compared. Port $F2 is left as it was found.
detect_fm:
    in a,($F2)
    and 3
    ld d,a
    ld b,0
detect_loop:
    ld a,b
    out ($F2),a
    in a,($F2)
    xor b
    and 3
    jr nz,detect_missing
    inc b
    ld a,b
    cp 4
    jr nz,detect_loop
    ld a,d
    out ($F2),a
    ld a,1
    ret
detect_missing:
    ld a,d
    out ($F2),a
    xor a
    ret

.INCLUDE "mailbox.inc"

table:
; Segment 1: instrument 1 on voices 0 to 2, as a chord one voice at a time,
; then voice 0 again an octave below. Register $3n is the instrument in the
; high nibble and the volume (0 loudest) in the low; $1n is the low eight
; bits of the f-number and $2n is bit 8 of it, the block in bits 3-1 and the
; key-on in bit 4 (SMS-AUDIO section 4.2).
    S_F2 3
    S_FM $30, $10
    S_FM $31, $10
    S_FM $32, $10
    S_FM $10, $80
    S_FM $20, $19
    S_WAIT 12
    S_FM $11, $A0
    S_FM $21, $19
    S_WAIT 12
    S_FM $12, $C0
    S_FM $22, $19
    S_WAIT 12
    S_FM $20, $09
    S_FM $21, $09
    S_FM $22, $09
    S_WAIT 10
    S_FM $10, $40
    S_FM $20, $17
    S_WAIT 10
    S_FM $20, $07
    S_WAIT 4
    S_MARK

; Segment 2: rhythm mode. Voices 6 to 8 carry the drums: their frequencies,
; then the five drum volumes (register $36 the bass drum, the high nibble of
; $37 the hi-hat and its low nibble the snare, the high nibble of $38 the
; tom-tom and its low nibble the cymbal). Register $0E holds the rhythm bit
; $20 and one key bit per drum: bass $10, snare $08, tom-tom $04, cymbal $02,
; hi-hat $01 (SMS-AUDIO section 4.2).
    S_FM $16, $20
    S_FM $17, $50
    S_FM $18, $C0
    S_FM $26, $05
    S_FM $27, $05
    S_FM $28, $01
    S_FM $36, $00
    S_FM $37, $00
    S_FM $38, $00
    S_FM $0E, $20
    S_FM $0E, $30
    S_WAIT 8
    S_FM $0E, $20
    S_WAIT 3
    S_FM $0E, $28
    S_WAIT 8
    S_FM $0E, $20
    S_WAIT 3
    S_FM $0E, $24
    S_WAIT 8
    S_FM $0E, $20
    S_WAIT 3
    S_FM $0E, $22
    S_WAIT 8
    S_FM $0E, $20
    S_WAIT 3
    S_FM $0E, $21
    S_WAIT 8
    S_FM $0E, $20
    S_WAIT 3
    S_FM $0E, $3F
    S_WAIT 8
    S_FM $0E, $20
    S_WAIT 3
    S_FM $0E, $00
    S_MARK

; Segment 3: voice 0 sounds instrument 1 and PSG tone 0 sounds at attenuation
; 6, while $F2 steps through 0, 1, 2 and 3 for 20 frames each.
    S_FM $30, $10
    S_FM $10, $80
    S_FM $20, $19
    S_PSG $8E
    S_PSG $0F
    S_PSG $96
    S_WAIT 2
    S_F2 0
    S_WAIT 20
    S_F2 1
    S_WAIT 20
    S_F2 2
    S_WAIT 20
    S_F2 3
    S_WAIT 20
    S_FM $20, $09
    S_PSG $9F
    S_WAIT 6
    S_MARK

    .DB OP_END
