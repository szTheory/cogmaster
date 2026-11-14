; Owned legacy sprite scenarios (MIT). A Z80 program that runs a fixed list of Graphics I sprite
; scenarios, one at a time, and after each reads the status register once and reports it through
; the RAM mailbox. It judges nothing: tests/diag/expected.csv holds the verdicts, every status
; row computed by legacy_status() in tests/fixtures/legacy.c from the same scenario list, and
; the runner's frame hash for each scenario must equal the digest legacy.c prints.
;
; Fault prevented: a legacy sprite engine that draws the right pictures but sets the wrong
; status flags, or one that gets the flags right only at the frame end: the fifth sprite flag
; and its index are carried from the scan of the previous line to dot 0 of the line, which only
; a read in between can see (the tail scenario).
;
; The background is Graphics I with every table zero, so the picture is the backdrop; R7's low
; nibble differs from scenario to scenario, which names the scenario in the picture. The sprite
; generator holds the generated patterns: name 0 blank, 1 solid, 2 the left column, 3 the right
; column, and (53 n + 17 r + 1) & 255 for any other name n at row r. R1 bit 1 selects 16 by 16
; sprites, bit 0 magnification; both are set per scenario with R0 and R7. The attribute table
; is 32 entries of Y, X, name and colour (bit 7 early clock); entries after those a scenario
; lists read Y $D0, which ends the list (TI TMS9918A manual 2.4.5).
;
; Timing. One scenario takes two frames. The program waits for vertical blank by polling the V
; counter (line 194, V $C2), reads the status once to clear what the last picture left, writes
; the scenario (registers and the 128-byte attribute table) and waits for the next vertical
; blank, where the one recorded status read happens: the frame flag is set (line 192, tick 607
; of the line) and every sprite flag of the picture is in. The status is never read in between,
; because a read clears the flags. The next scenario is written one frame later, so each
; picture is on screen for two frames.
;
; The tail scenario (the last) enables the line interrupt (R0 bit 4) on line 79 (R10). Four
; sprites start on line 81 and a fifth ends on it, so line 81 has five Y matches and no other
; line has more than one. The scan for line 81 runs at tick 600 of line 80, and the fifth
; sprite flag asserts at dot 0 of line 81, tick 684. The interrupt handler waits TAIL_DELAY
; loops, reads the status once (after the scan, before dot 0) and reads the H counter 45 ticks
; later. The frame-end read must still report the fifth sprite's index. The delay is calibrated
; until the H counter lies in the range legacy.c names TAIL_HCOUNT_LO to TAIL_HCOUNT_HI.
;
; Mailbox records: id n (1 to 19) the status at the end of scenario n; 20 the status read in
; the tail of line 80; 21 the H counter next to it; 22 the status at the end of the tail
; scenario.
.INCLUDE "header.inc"
.DEFINE TAIL_DELAY 13
.BANK 0 SLOT 0
.ORG 0
    di
    ld sp,$DFF0
    jp main
.ORG $38
    jp tail_irq
.ORG $66
    retn
main:
    ld a,9
    call mb_init

    ; Colour RAM: 32 entries, (13 i + 5) & 63, as legacy_modes.
    xor a
    out ($BF),a
    ld a,$C0
    out ($BF),a
    ld e,5
    ld b,32
cram_loop:
    ld a,e
    and 63
    out ($BE),a
    ld a,e
    add a,13
    ld e,a
    djnz cram_loop

    ; Clear video memory $0000 to $1FFF: names $0000, patterns $0800, colours $1000, the
    ; attribute table $1400 and the sprite generator $1800 are all inside.
    ld hl,$0000
    call vram_write_at
    ld bc,$2000
clear_vram:
    xor a
    out ($BE),a
    dec bc
    ld a,b
    or c
    jr nz,clear_vram

    ; Sprite generator at $1800: 256 names of 8 rows, byte (53 n + 17 r + 1) & 255, then names
    ; 0 to 3 are replaced by blank, solid, the left column and the right column.
    ld hl,$1800
    call vram_write_at
    ld e,1
    ld d,0
sprite_name:
    ld a,e
    ld b,8
sprite_row:
    out ($BE),a
    add a,17
    djnz sprite_row
    ld a,e
    add a,53
    ld e,a
    dec d
    jr nz,sprite_name
    ld hl,$1800
    call vram_write_at
    xor a
    call fill8
    ld a,255
    call fill8
    ld a,128
    call fill8
    ld a,1
    call fill8

    ; Registers R2 to R10, then R0, then R1 with the display on.
    ld hl,registers
    ld b,9
    ld c,$82
register_loop:
    ld a,(hl)
    out ($BF),a
    ld a,c
    out ($BF),a
    inc hl
    inc c
    djnz register_loop
    ld a,$00
    out ($BF),a
    ld a,$80
    out ($BF),a
    ld a,$C0
    out ($BF),a
    ld a,$81
    out ($BF),a

    ; Scenarios 1 to 19: $C230 the record id, $C232 the next scenario record.
    ld hl,scenarios
    ld ($C232),hl
    ld a,1
    ld ($C230),a
scenario_loop:
    call wait_frame
    in a,($BF)
    ld hl,($C232)
    call load_scenario
    ld ($C232),hl
    call wait_frame
    in a,($BF)
    ld l,a
    ld h,0
    ld a,($C230)
    call mb_record
    ld a,($C230)
    inc a
    ld ($C230),a
    cp 20
    jr nz,scenario_loop

    ; Scenario 20, the tail: the line interrupt on line 79 records the tail read.
    call wait_frame
    in a,($BF)
    ld hl,($C232)
    call load_scenario
    ld a,79
    out ($BF),a
    ld a,$8A
    out ($BF),a
    im 1
    ei
    halt
    di
    call wait_frame
    in a,($BF)
    ld l,a
    ld h,0
    ld a,22
    call mb_record
    ld a,($C220)
    ld l,a
    ld h,0
    ld a,20
    call mb_record
    ld a,($C221)
    ld l,a
    ld h,0
    ld a,21
    call mb_record
    call mb_done
done:
    jr done

; The line interrupt of the tail scenario. It reads the status exactly once, after a calibrated
; delay, then the H counter, and stores both ($C220 and $C221); the status read also releases
; the interrupt. Then it turns the line interrupt off (R0 $00).
tail_irq:
    push af
    push bc
    ld b,TAIL_DELAY
tail_delay:
    djnz tail_delay
    nop
    nop
    in a,($BF)
    ld c,a
    in a,($7F)
    ld ($C221),a
    ld a,c
    ld ($C220),a
    ld a,$00
    out ($BF),a
    ld a,$80
    out ($BF),a
    pop bc
    pop af
    ei
    reti

; Waits for the next vertical blank: V $C2 is line 194, after the frame flag (line 192).
wait_frame:
wait_leave:
    in a,($7E)
    cp $C2
    jr z,wait_leave
wait_enter:
    in a,($7E)
    cp $C2
    jr nz,wait_enter
    ret

; HL = a scenario record. Writes R7, R0 and R1, then the 32-entry attribute table: the listed
; entries, then Y $D0 entries. Returns HL at the next record.
load_scenario:
    ld a,(hl)
    out ($BF),a
    ld a,$87
    out ($BF),a
    inc hl
    ld a,(hl)
    out ($BF),a
    ld a,$80
    out ($BF),a
    inc hl
    ld a,(hl)
    or $C0
    out ($BF),a
    ld a,$81
    out ($BF),a
    inc hl
    ld d,(hl)
    inc hl
    xor a
    out ($BF),a
    ld a,$54
    out ($BF),a
    ld a,d
    add a,a
    add a,a
    ld b,a
    ld c,$BE
    otir
    ld a,32
    sub d
    ld b,a
pad_table:
    ld a,$D0
    out ($BE),a
    xor a
    out ($BE),a
    out ($BE),a
    out ($BE),a
    djnz pad_table
    ret

; A = byte: eight copies into video memory at the address already set.
fill8:
    ld b,8
fill8_loop:
    out ($BE),a
    djnz fill8_loop
    ret

; HL = video memory address: the data port writes that follow go there.
vram_write_at:
    ld a,l
    out ($BF),a
    ld a,h
    or $40
    out ($BF),a
    ret

; R2 to R10: names $0000, colours $1000, patterns $0800, attribute table $1400, sprite
; generator $1800, backdrop 2, no scroll, R10 $FF (no line interrupt in the picture).
registers:
    .DB $00,$40,$01,$28,$03,$02,$00,$00,$FF

; One record per scenario: R7 backdrop, R0, R1 low bits, entry count, then entries of Y, X,
; name and colour. Generated from the scenario list of tests/fixtures/legacy.c, in order.
scenarios:
    ; 1 single_8x8
    .DB 2,0,0,1
    .DB 39,100,1,5
    ; 2 size_16x16
    .DB 3,0,2,1
    .DB 39,100,8,7
    ; 3 magnified_8x8
    .DB 4,0,1,1
    .DB 39,100,9,9
    ; 4 magnified_16x16
    .DB 5,0,3,1
    .DB 39,100,8,11
    ; 5 early_clock
    .DB 6,8,0,3
    .DB 15,70,1,130
    .DB 47,70,3,2
    .DB 79,36,1,132
    ; 6 end_of_list
    .DB 7,0,0,3
    .DB 19,40,1,5
    .DB 208,0,0,0
    .DB 19,120,1,6
    ; 7 y_ff
    .DB 8,0,0,1
    .DB 255,60,12,4
    ; 8 y_e8_magnified_16x16
    .DB 9,0,3,1
    .DB 232,60,8,5
    ; 9 y_e0_magnified_16x16
    .DB 10,0,3,1
    .DB 224,60,8,6
    ; 10 five_on_a_line
    .DB 11,0,0,5
    .DB 39,20,1,2
    .DB 39,40,1,3
    .DB 39,60,1,4
    .DB 39,80,1,5
    .DB 39,100,1,6
    ; 11 overflow_two_lines
    .DB 12,0,0,10
    .DB 99,20,1,2
    .DB 99,40,1,3
    .DB 99,60,1,4
    .DB 99,80,1,5
    .DB 99,100,1,6
    .DB 39,20,1,7
    .DB 39,40,1,8
    .DB 39,60,1,9
    .DB 39,80,1,10
    .DB 39,100,1,11
    ; 12 overlap_opaque
    .DB 13,0,0,2
    .DB 59,100,1,5
    .DB 59,104,1,6
    ; 13 overlap_colour0
    .DB 14,0,0,2
    .DB 59,100,1,0
    .DB 59,104,1,0
    ; 14 touching
    .DB 2,0,0,2
    .DB 59,100,1,5
    .DB 59,108,1,6
    ; 15 overlap_off_left_edge
    .DB 3,0,0,3
    .DB 59,10,1,133
    .DB 59,14,1,134
    .DB 99,100,1,7
    ; 16 after_terminator
    .DB 4,0,0,4
    .DB 19,40,1,3
    .DB 208,0,0,0
    .DB 59,100,1,5
    .DB 59,104,1,6
    ; 17 fifth_overlaps
    .DB 5,0,0,5
    .DB 59,20,1,2
    .DB 59,40,1,3
    .DB 59,60,1,4
    .DB 59,80,1,5
    .DB 59,84,1,6
    ; 18 four_on_a_line
    .DB 6,0,0,4
    .DB 59,20,1,2
    .DB 59,50,1,3
    .DB 59,80,1,4
    .DB 59,110,1,5
    ; 19 magnified_overlap
    .DB 7,0,1,2
    .DB 59,100,1,5
    .DB 59,108,1,6
    ; 20 tail
    .DB 8,16,0,5
    .DB 80,20,1,2
    .DB 80,50,1,3
    .DB 80,80,1,4
    .DB 80,110,1,5
    .DB 73,140,1,6
.INCLUDE "mailbox.inc"
