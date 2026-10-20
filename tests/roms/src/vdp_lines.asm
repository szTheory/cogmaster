; Owned diagonal tiles, scroll bands and numeric interrupt measurements.
; Geometry/timing derivation: docs/hardware/video.md, SMS-VDP sections 2-5.
.INCLUDE "header.inc"
.IFNDEF LINES_HEIGHT
.DEFINE LINES_HEIGHT 192
.ENDIF
.IF LINES_HEIGHT == 224
.DEFINE MODE_BITS $10
.DEFINE MAP_HIGH $77
.ELSE
.IF LINES_HEIGHT == 240
.DEFINE MODE_BITS $08
.DEFINE MAP_HIGH $77
.ELSE
.DEFINE MODE_BITS $00
.DEFINE MAP_HIGH $78
.ENDIF
.ENDIF
.BANK 0 SLOT 0
.ORG 0
    di
    ld sp,$DFF0
    jp main
.ORG $38
    jp interrupt
.ORG $66
    retn
main:
    ld a,6
    call mb_init
    xor a
    ld ($C210),a
    ld ($C211),a
    ; Upload four planar tiles at $0000.
    out ($BF),a
    ld a,$40
    out ($BF),a
    ld hl,tiles
    ld b,128
    ld c,$BE
    otir
    xor a
    out ($BF),a
    ld a,MAP_HIGH
    out ($BF),a
.IF LINES_HEIGHT == 192
    ld bc,896
.ELSE
    ld bc,1024
.ENDIF
names:
    ld a,1
    out ($BE),a
    xor a
    out ($BE),a
    dec bc
    ld a,b
    or c
    jr nz,names
    ; Row one columns 2..5: horizontal flip, vertical flip, palette, priority.
    ld a,$44
    out ($BF),a
    ld a,MAP_HIGH
    out ($BF),a
    ld hl,attributes
    ld b,8
    ld c,$BE
    otir
    ; Two overlapping zoomed 8x16 sprites, and two later 8x8 sprites.
    xor a
    out ($BF),a
    ld a,$7F
    out ($BF),a
    ld a,$FF
    out ($BE),a
    out ($BE),a
    ld a,95
    out ($BE),a
    out ($BE),a
    ld b,60
    ld a,$EF
unused_y:
    out ($BE),a
    djnz unused_y
    ld a,$80
    out ($BF),a
    ld a,$7F
    out ($BF),a
    ld hl,sprites
    ld b,8
    ld c,$BE
    otir
    xor a
    out ($BF),a
    ld a,$C0
    out ($BF),a
    ld hl,colours
    ld b,32
    ld c,$BE
    otir
    ld a,$7E
    out ($BF),a
    ld a,$85
    out ($BF),a
    ld a,1
    out ($BF),a
    ld a,$87
    out ($BF),a
    ld a,8
    out ($BF),a
    ld a,$88
    out ($BF),a
    ld a,8
    out ($BF),a
    ld a,$89
    out ($BF),a
    ld a,31
    out ($BF),a
    ld a,$8A
    out ($BF),a
    ; M4/M2, mask, both locks and line IE. Both sizes appear in every field.
    ld a,$F6
    out ($BF),a
    ld a,$80
    out ($BF),a
    ld a,$63 | MODE_BITS
    out ($BF),a
    ld a,$81
    out ($BF),a
    in a,($BF)
    im 1
    ei
wait_four:
    halt
    ld a,($C210)
    cp 4
    jr c,wait_four
    di
    ld a,1
    ld hl,($C212)
    ld h,0
    call mb_record
    ld a,($C210)
    ld l,a
    ld h,0
    ld a,2
    call mb_record
    ld a,($C213)
    ld l,a
    ld h,0
    ld a,3
    call mb_record
    ld a,($C214)
    ld l,a
    ld h,0
    ld a,4
    call mb_record
    ld a,($C215)
    ld l,a
    ld h,0
    ld a,5
    call mb_record
    ld a,($C216)
    ld l,a
    ld h,0
    ld a,6
    call mb_record
    ; Poll one selected visible-counter boundary with IRQs disabled. These
    ; are numeric samples, not a ROM-side assertion of correctness.
wait_before:
    in a,($7E)
    cp $3F
    jr nz,wait_before
    ld ($C217),a
wait_after:
    in a,($7E)
    cp $3F
    jr z,wait_after
    ld ($C218),a
    ld a,($C217)
    ld l,a
    ld h,0
    ld a,7
    call mb_record
    ld a,($C218)
    ld l,a
    ld h,0
    ld a,8
    call mb_record
    call mb_done
    ; Keep raster bands running after sealing the measurements.
    ei
forever:
    halt
    jr forever
interrupt:
    push af
    in a,($BF)
    bit 7,a
    jr z,line_interrupt
    ld ($C215),a
    in a,($7F)
    ld ($C213),a
    in a,($7E)
    ld ($C214),a
    in a,($BF)
    ld ($C216),a
    ld a,($C211)
    ld ($C212),a
    xor a
    ld ($C211),a
    ld a,($C210)
    inc a
    ld ($C210),a
    ld a,8
    out ($BF),a
    ld a,$88
    out ($BF),a
    ld a,$63 | MODE_BITS
    out ($BF),a
    ld a,$81
    out ($BF),a
    jr interrupt_done
line_interrupt:
    ld a,($C211)
    inc a
    ld ($C211),a
    inc a
    add a,a
    add a,a
    add a,a
    out ($BF),a
    ld a,$88
    out ($BF),a
    ld a,($C211)
    cp 2
    jr nz,interrupt_done
    ld a,$60 | MODE_BITS
    out ($BF),a
    ld a,$81
    out ($BF),a
interrupt_done:
    pop af
    ei
    reti
tiles:
    .DSB 32,0
    .DB $80,0,0,0,$40,0,0,0,$20,0,0,0,$10,0,0,0
    .DB $08,0,0,0,$04,0,0,0,$02,0,0,0,$01,0,0,0
    .REPT 8
    .DB 0,$FF,0,0
    .ENDR
    .REPT 8
    .DB $FF,$FF,0,0
    .ENDR
attributes:
    .DB 1,2,1,4,1,8,1,$10
sprites:
    .DB 40,2,48,2,72,2,72,3
colours:
    .DB 0,3,12,$3F,0,0,0,0,0,0,0,0,0,0,0,0
    .DB 0,$30,12,$3F,0,0,0,0,0,0,0,0,0,0,0,0
.INCLUDE "mailbox.inc"
