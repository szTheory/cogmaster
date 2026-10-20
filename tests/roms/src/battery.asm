; Owned Sega-board battery continuation fixture. RAM bank selection follows
; SMS-CARTRIDGES section 2.1; only the first three bytes persist the counter.
.DEFINE FIXTURE_MAP
.MEMORYMAP
DEFAULTSLOT 0
SLOT 0 START $0000 SIZE $4000
.ENDME
.ROMBANKMAP
BANKSTOTAL 4
BANKSIZE $4000
BANKS 4
.ENDRO
.EMPTYFILL $FF
.INCLUDE "header.inc"
.BANK 0 SLOT 0
.ORG 0
    di
    ld sp,$DFF0
    jp main
.ORG $66
    retn
main:
    ld a,7
    call mb_init
    ld a,$08
    ld ($FFFC),a
    ld a,($8000)
    cp $A5
    jr nz,fresh
    ld a,($8001)
    cp $5A
    jr nz,fresh
    ld a,($8002)
    jr advance
fresh:
    ld a,$A5
    ld ($8000),a
    ld a,$5A
    ld ($8001),a
    xor a
advance:
    ld l,a
    inc a
    ld ($8002),a
    ld h,a
    ld a,1
    call mb_record
    call mb_done
done:
    jr done
.INCLUDE "mailbox.inc"
