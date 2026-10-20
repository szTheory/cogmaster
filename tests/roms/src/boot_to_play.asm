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
    xor a
    ld ($C200),a
    ld ($C201),a
    ld ($C202),a
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
.IFDEF FIXTURE_GG
    ld hl,gg_palette
    ld b,64
    ld c,$BE
    otir
.ELSE
    ld b,32
    xor a
palette:
    out ($BE),a
    inc a
    djnz palette
.ENDIF
    ; Terminate a dedicated SAT before enabling the background display.
    xor a
    out ($BF),a
    ld a,$7F
    out ($BF),a
    ld a,$D0
    out ($BE),a
    ld a,$7F
    out ($BF),a
    ld a,$85
    out ($BF),a
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
forever:
    ; One poll in VBlank per frame; bit 4 is active-low button 1.
.IFDEF FIXTURE_GG
    in a,($00)
    ld ($C204),a
    cpl
    rrca
    rrca
    rrca
    and $10
    ld b,a
.ELSE
    in a,($DC)
    cpl
    and $10
    ld b,a
    in a,($DD)
    cpl
    and 4
    rlca
    rlca
    or b
    ld b,a
.ENDIF
    ld a,($C201)
    xor b
    and b
    ld c,a
    ld a,b
    ld ($C201),a
    ld a,c
    or a
    jr z,no_edge
    ld a,($C200)
    inc a
    ld ($C200),a
    out ($BF),a
    ld a,$88
    out ($BF),a
    ; Palette entry 15 is the visible colour counter, six-bit SMS CRAM.
.IFDEF FIXTURE_GG
    ld a,30
.ELSE
    ld a,15
.ENDIF
    out ($BF),a
    ld a,$C0
    out ($BF),a
    ld a,($C200)
.IFDEF FIXTURE_GG
    ; Counter values 1..3 widen exactly into the red nibble.
    ld b,a
    add a,a
    add a,a
    add a,b
.ENDIF
    out ($BE),a
.IFDEF FIXTURE_GG
    xor a
    out ($BE),a
.ENDIF
no_edge:
    ; Record the first eight polls, then seal once. CRC calculation never
    ; delays any of the press/hold/release measurements.
    ld a,($C202)
    cp 8
    jr nc,sampled
    inc a
    ld ($C202),a
    push af
    ld a,($C200)
    ld h,a
    ld l,a
    pop af
    call mb_record
.IFDEF FIXTURE_GG
    ld a,($C204)
    ld l,a
    ld h,0
    ld a,($C202)
    add a,$10
    call mb_record
.ENDIF
    ld a,($C202)
    cp 8
    jr nz,sampled
    call mb_done
sampled:
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
.IFDEF FIXTURE_GG
gg_palette:
    .DB $00,$00,$05,$00,$0A,$00,$0F,$00
    .DB $50,$00,$55,$00,$5A,$00,$5F,$00
    .DB $A0,$00,$A5,$00,$AA,$00,$AF,$00
    .DB $F0,$00,$F5,$00,$FA,$00,$FF,$00
    .DB $00,$05,$05,$05,$0A,$05,$0F,$05
    .DB $50,$05,$55,$05,$5A,$05,$5F,$05
    .DB $A0,$05,$A5,$05,$AA,$05,$AF,$05
    .DB $F0,$05,$F5,$05,$FA,$05,$FF,$05
.ENDIF
.INCLUDE "mailbox.inc"
