; Own four-plane glyph: a diamond in plane 0, with three vertical bars.
; Name table $3800 is all tile 0. Palette entries are their own SMS value.
.INCLUDE "header.inc"
.BANK 0 SLOT 0
.ORG 0
    di
    ld sp,$DFF0
    jp main
.ORG $66
    retn
main:
    ld a,5
    call mb_init
    xor a
    out ($BF),a
    ld a,$40
    out ($BF),a
    ld hl,glyph
    ld b,32
    ld c,$BE
    otir
    xor a
    out ($BF),a
    ld a,$78
    out ($BF),a
    ld bc,1792
clear_names:
    xor a
    out ($BE),a
    dec bc
    ld a,b
    or c
    jr nz,clear_names
    xor a
    out ($BF),a
    ld a,$C0
    out ($BF),a
    ld b,32
    xor a
palette:
    out ($BE),a
    inc a
    djnz palette
    ld a,4
    out ($BF),a
    ld a,$80
    out ($BF),a
    ld a,$40
    out ($BF),a
    ld a,$81
    out ($BF),a
    ; Wait two complete frame flags before reporting a stable picture.
    in a,($BF)
    call wait_frame
    call wait_frame
    ld hl,$0F01
    ld a,1
    call mb_record
    call mb_done
forever:
    call wait_frame
    jr forever
wait_frame:
    in a,($BF)
    or a
    jp p,wait_frame
    ret
glyph:
    .DB $81,$55,$33,$0F
    .DB $42,$55,$33,$0F
    .DB $24,$55,$33,$0F
    .DB $18,$55,$33,$0F
    .DB $18,$55,$33,$0F
    .DB $24,$55,$33,$0F
    .DB $42,$55,$33,$0F
    .DB $81,$55,$33,$0F
.INCLUDE "mailbox.inc"
