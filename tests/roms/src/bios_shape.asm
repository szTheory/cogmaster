; An owned, BIOS-shaped test image and its probe cartridge, four builds of one source.
;
; Fault prevented: a BIOS-shaped image that is a derivative of firmware (this one is written from
; the public hand-off facts alone, SMS-SYSTEM sections 3 and 6 and C7, carries its own marker
; text and no logo, and never the header text the cartridge scans look for), and a CI run with
; nothing BIOS-shaped to run, so the boot path would be tested only by images that nobody can
; tell from a cartridge.
;
; Builds (wla-z80 -D selectors; the link file's object name is rewritten per image):
;   PROBE    the header-less probe cartridge: di then halt at $0000, 64 KiB. The CPU freezes
;            there, so a test can read the machine at the hand-off instant. Sixteen stores to
;            $FFFF sit after the halt, never run, so the load takes the Sega mapper and the
;            power-on banks 0, 1, 2 show (a 16 KiB image would wrap them all to bank 0).
;   (none)   bios_shape.bin, 8,192 bytes: the whole image below.
;   BAD_RAM  the same with the store of $AB at $C000 left out.
;   BAD_SP   the same with SP $DFF2 where $DFF0 belongs.
;
; What the image does. Power-on has port $3E at $E0 (BIOS, work RAM and I/O on; the cartridge,
; card and expansion off), so the code starts at $0000 and gives the VDP a visible backdrop
; through CRAM entry 17 and R7, and a picture colour through CRAM entry 0, with the display on,
; which a fast boot does not (its frames are black).
; It then copies one program to work RAM inside $C100-$C1FF and jumps there, because the BIOS
; ROM disables itself and cannot keep executing from its own window. The program:
;   1. walks the enable combinations (BIOS on and cartridge off, neither, cartridge on and BIOS
;      off, each with RAM on), recording reads of $0000 and $0001 and a write then read of
;      $FFFC-$FFFF in the scratch results at $C1D0. The mapper and the work RAM mirror bytes it
;      wrote are put back at once, so the walk's only trace in compared state is in the window.
;   2. waits about two frames, then puts R0, R1, R7 and CRAM entries 0 and 17 back to 0.
;   3. zeroes every register pair (main, shadow, IX, IY), writes $3F = $FF and $3E = $AB,
;      stores $AB at $C000, loads AF = $FFFF by push and pop with SP first inside the window,
;      sets SP $DFF0 and jumps to $0000: the hand-off a Master System BIOS gives a cartridge.
; Everything written to work RAM except $C000 and the mirror bytes lies in $C100-$C1FF: the
; code at $C100, the results at $C1D0, the stack below $C1F0. It never sets IM, so IM stays 0.

.IFDEF PROBE
.DEFINE LAST_BANK 3
.DEFINE LAST_ORG $3FF0
.MEMORYMAP
DEFAULTSLOT 0
SLOT 0 START $0000 SIZE $4000
.ENDME
.ROMBANKMAP
BANKSTOTAL 4
BANKSIZE $4000
BANKS 4
.ENDRO
.EMPTYFILL $00
.ELSE
.DEFINE LAST_BANK 0
.DEFINE LAST_ORG $1FF0
.MEMORYMAP
DEFAULTSLOT 0
SLOT 0 START $0000 SIZE $2000
.ENDME
.ROMBANKMAP
BANKSTOTAL 1
BANKSIZE $2000
BANKS 1
.ENDRO
.EMPTYFILL $FF
.ENDIF

.BANK 0 SLOT 0
.ORG 0
.IFDEF PROBE
    di
    halt
.ORG $100
    .REPT 16
    ld ($FFFF),a
    .ENDR
.ELSE
    di
    jp start
; The non-maskable interrupt (Pause) returns at once: the image never enables interrupts.
.ORG $66
    retn
; The marker: owned text, so nobody mistakes this image for firmware.
.ORG $100
    .DB "COGMASTER OWNED TEST IMAGE, NOT A BIOS"
.ORG $200
start:
    ; The colours: CRAM entry 0 paints the picture, entry 17 is the backdrop that R7 picks. R0
    ; turns mode 4 on and R1 the display.
    xor a
    out ($BF),a
    ld a,$C0
    out ($BF),a
    ld a,$0C
    out ($BE),a
    ld a,$11
    out ($BF),a
    ld a,$C0
    out ($BF),a
    ld a,$30
    out ($BE),a
    ld a,$04
    out ($BF),a
    ld a,$80
    out ($BF),a
    ld a,$01
    out ($BF),a
    ld a,$87
    out ($BF),a
    ld a,$40
    out ($BF),a
    ld a,$81
    out ($BF),a
    ld hl,ram_start
    ld de,$C100
    ld bc,ram_end - ram_start
    ldir
    jp $C100

; The program that runs from work RAM at $C100. Only relative jumps and label differences are
; used, so it assembles here and runs there unchanged.
ram_start:
    ld sp,$C1F0
    ld de,$C1D0
    ld a,$E0
    call probe - ram_start + $C100
    ld a,$C8
    call probe - ram_start + $C100
    ld a,$A8
    call probe - ram_start + $C100
    ; The cartridge is on: put the mapper back to its power-on values, then the mirror bytes.
    xor a
    ld ($FFFC),a
    ld ($FFFD),a
    inc a
    ld ($FFFE),a
    inc a
    ld ($FFFF),a
    xor a
    ld ($DFFC),a
    ld ($DFFD),a
    ld ($DFFE),a
    ld ($DFFF),a
    ; About two frames: 4,595 passes of a 26 cycle loop against 59,736 cycles a frame.
    ld bc,4595
pause:
    dec bc
    ld a,b
    or c
    jr nz,pause
    ; The VDP back to its reset registers and CRAM entries 0 and 17 back to 0.
    xor a
    out ($BF),a
    ld a,$80
    out ($BF),a
    xor a
    out ($BF),a
    ld a,$81
    out ($BF),a
    xor a
    out ($BF),a
    ld a,$87
    out ($BF),a
    xor a
    out ($BF),a
    ld a,$C0
    out ($BF),a
    xor a
    out ($BE),a
    ld a,$11
    out ($BF),a
    ld a,$C0
    out ($BF),a
    xor a
    out ($BE),a
    ; Every register pair to zero: AF by push and pop, the shadow set through EX and EXX.
    ld hl,0
    push hl
    pop af
    ex af,af'
    ld bc,0
    ld de,0
    exx
    ld bc,0
    ld de,0
    ld hl,0
    exx
    ld ix,0
    ld iy,0
    ; The ports and the byte at the top of work RAM.
    ld a,$FF
    out ($3F),a
    ld a,$AB
    out ($3E),a
.IFNDEF BAD_RAM
    ld ($C000),a
.ENDIF
    ; AF = $FFFF by push and pop (SP is inside the window), then HL back to 0 and SP to its value.
    ld hl,$FFFF
    push hl
    pop af
    ld hl,0
.IFDEF BAD_SP
    ld sp,$DFF2
.ELSE
    ld sp,$DFF0
.ENDIF
    jp $0000

; A = the $3E value to try. Reads $0000 and $0001, writes $20 to $23 to $FFFC-$FFFF and reads
; them back, appending the six bytes at DE. The values keep every bit the Sega mapper treats as
; unused clear, so the write is no fault when the cartridge is on.
probe:
    out ($3E),a
    ld a,($0000)
    ld (de),a
    inc de
    ld a,($0001)
    ld (de),a
    inc de
    ld hl,$FFFC
    ld b,4
    ld a,$20
probe_write:
    ld (hl),a
    inc a
    inc hl
    djnz probe_write
    ld hl,$FFFC
    ld b,4
probe_read:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    djnz probe_read
    ret
ram_end:
.ENDIF

; The last 16 bytes, where a cartridge header would sit, are filled by directive: this image has
; none, and the scans that look for one must find none.
.BANK LAST_BANK SLOT 0
.ORG LAST_ORG
.dsb 16,$FF
