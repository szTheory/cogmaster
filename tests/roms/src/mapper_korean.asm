; The Korean-family mapper fixture: one source, three 128 KiB images of sixteen
; 8 KiB banks, chosen by FIXTURE_BOARD: 1 Korean, 2 Zemina, 3 4 PAK. The images
; carry no ROM header, so the runner finds each board by the instruction scan
; of the first 32 KiB (at least ten LD (nn),A stores to the board's own
; registers, none to another family's). Each bank holds a start stamp at $0080:
; its number, the number's complement and 14 bytes of text, and an end stamp at
; $1FF0: $80 plus the number, $7F minus it and 14 bytes of text. The ROM pages
; the banks and reports what it reads through the RAM mailbox; it judges
; nothing. S(w) below is the stamp number read at the start of the 8 KiB
; window w: window 0 is $0000, 1 $2000, 2 $4000, 3 $6000, 4 $8000, 5 $A000.
;
; The boards (MAME src/devices/bus/sms_cart/rom.cpp, pin 1d7a454ed79e;
; SMS-CARTRIDGES section 2.4 for the wrap): 128 KiB is 8 pages of 16 KiB or
; 16 banks of 8 KiB, so a page number wraps modulo 8 and a bank number modulo
; 16.
;
; Korean: a write to exactly $A000 picks the 16 KiB page of $8000-$BFFF;
; windows 0-3 stay on banks 0-3 and banks 0-5 show at reset. $FFFC-$FFFF do
; nothing.
;   01  S(0) | S(1) << 8 at reset        02  S(2) | S(3) << 8        03  S(4) | S(5) << 8
;   04  S(4) | S(5) << 8 after $A000 = 3         05  the same after 7
;   06  the same after 10 (page 2)               07  the same after $FF (page 7)
;   08  after $A000 = 5: S(0) | S(1) << 8        09  S(2) | S(3) << 8
;   0A  after writes of 1, 2, 3, 4 to $FFFC-$FFFF: S(0) | S(1) << 8
;   0B  S(2) | S(3) << 8                         0C  S(4) | S(5) << 8
;   0D  sum of S(4), then of S(5) << 8, over pages 0-7
;
; Zemina: writes to $0000, $0001, $0002 and $0003 pick the 8 KiB banks of
; $8000, $A000, $4000 and $6000; $0000-$3FFF stay on banks 0 and 1, and banks
; 0-5 show at reset. $FFFC-$FFFF do nothing.
;   01-03  as Korean                             04  after writes of 6, 7, 8, 9: S(4) | S(5) << 8
;   05  S(2) | S(3) << 8                         06  after writes of 27, $FF, 28, 29: S(4) | S(5) << 8
;   07  S(2) | S(3) << 8                         08  S(0) | S(1) << 8
;   09-0B  after writes to $FFFC-$FFFF: S(0) | S(1) << 8, S(2) | S(3) << 8, S(4) | S(5) << 8
;   0C  sum of S(4), then of S(5) << 8, over banks 0-15
;
; 4 PAK: $3FFE, $7FFF and $BFFF pick the 16 KiB pages of $0000-$3FFF,
; $4000-$7FFF and $8000-$BFFF (the last also takes the high bits of the $3FFE
; value, which a 128 KiB image cannot show); every page is 0 at reset. The
; board inherits the Sega registers MAME's sega8_rom_device::write_mapper
; implements, so a write to $FFFF picks the page of $8000-$BFFF as well. Slot 0
; moves with $3FFE, so that write runs from work RAM.
;   01  S(0) | S(1) << 8 at reset        02  S(2) | S(3) << 8        03  S(4) | S(5) << 8
;   04  S(2) | S(3) << 8 after $7FFF = 5         05  S(4) | S(5) << 8 after $BFFF = 3
;   06  S(0) | S(1) << 8 after $3FFE = 4         07  the same after $3FFE = 14 (page 6)
;   08  S(2) | S(3) << 8 after $7FFF = 10        09  S(4) | S(5) << 8 after $BFFF = $FF
;   0A  S(4) | S(5) << 8 after $FFFF = 5
;   0B  sum of S(4), then of S(5) << 8, over $BFFF pages 0-7
;
; At the end the image paints the backdrop with the stamp number read through a
; paged window and turns the display on, so a wrong bank changes the frame.

.IFNDEF FIXTURE_BOARD
.DEFINE FIXTURE_BOARD 1
.ENDIF

.MEMORYMAP
DEFAULTSLOT 0
SLOT 0 START $0000 SIZE $2000
.ENDME

.ROMBANKMAP
BANKSTOTAL 16
BANKSIZE $2000
BANKS 16
.ENDRO

.EMPTYFILL $FF

.IF FIXTURE_BOARD == 1
.DEFINE ROM_ID 21
.ENDIF
.IF FIXTURE_BOARD == 2
.DEFINE ROM_ID 22
.ENDIF
.IF FIXTURE_BOARD == 3
.DEFINE ROM_ID 23
.ENDIF
.DEFINE STAMP $0080
.DEFINE STAMP_END $1FF0
; Where the 4 PAK's slot-0 code runs from: work RAM above the mailbox.
.DEFINE STUB $C200

.MACRO START_STAMP ARGS N
.ORG STAMP
    .DB N, $FF-N, "cogmaster bank"
.ENDM

.MACRO END_STAMP ARGS N
.ORG STAMP_END
    .DB $80+N, $7F-N, "cogmaster ends"
.ENDM

; Records the stamp numbers of windows LO and HI as one value, with test id ID.
.MACRO PAIR ARGS LO HI ID
    ld a,(LO*$2000+STAMP)
    ld l,a
    ld a,(HI*$2000+STAMP)
    ld h,a
    ld a,ID
    call mb_record
.ENDM

; Adds the stamp numbers of windows 4 and 5 to B and C after writing N to the
; registers RLO and RHI.
.MACRO SWEEP ARGS N RLO RHI
    ld a,N
    ld (RLO),a
    ld (RHI),a
    ld a,($8000+STAMP)
    add a,b
    ld b,a
    ld a,($A000+STAMP)
    add a,c
    ld c,a
.ENDM

.BANK 0 SLOT 0
.ORG $0000
    di
    ld sp,$DFF0
    jp main

; A Pause press changes nothing.
.ORG $0066
    retn

    START_STAMP 0

.ORG $0090
main:
    ld a,ROM_ID
    call mb_init

    PAIR 0 1 $01
    PAIR 2 3 $02
    PAIR 4 5 $03

.IF FIXTURE_BOARD == 1
    ld a,3
    ld ($A000),a
    PAIR 4 5 $04
    ld a,7
    ld ($A000),a
    PAIR 4 5 $05
    ld a,10
    ld ($A000),a
    PAIR 4 5 $06
    ld a,$FF
    ld ($A000),a
    PAIR 4 5 $07
    ld a,5
    ld ($A000),a
    PAIR 0 1 $08
    PAIR 2 3 $09
    call sega_writes
    PAIR 0 1 $0A
    PAIR 2 3 $0B
    PAIR 4 5 $0C
    ld bc,0
    SWEEP 0 $A000 $A000
    SWEEP 1 $A000 $A000
    SWEEP 2 $A000 $A000
    SWEEP 3 $A000 $A000
    SWEEP 4 $A000 $A000
    SWEEP 5 $A000 $A000
    SWEEP 6 $A000 $A000
    SWEEP 7 $A000 $A000
    ld l,b
    ld h,c
    ld a,$0D
    call mb_record
    ld a,5
    ld ($A000),a
.ENDIF

.IF FIXTURE_BOARD == 2
    ld a,6
    ld ($0000),a
    ld a,7
    ld ($0001),a
    ld a,8
    ld ($0002),a
    ld a,9
    ld ($0003),a
    PAIR 4 5 $04
    PAIR 2 3 $05
    ld a,16+11
    ld ($0000),a
    ld a,$FF
    ld ($0001),a
    ld a,16+12
    ld ($0002),a
    ld a,16+13
    ld ($0003),a
    PAIR 4 5 $06
    PAIR 2 3 $07
    PAIR 0 1 $08
    call sega_writes
    PAIR 0 1 $09
    PAIR 2 3 $0A
    PAIR 4 5 $0B
    ld bc,0
    SWEEP 0 $0000 $0001
    SWEEP 1 $0000 $0001
    SWEEP 2 $0000 $0001
    SWEEP 3 $0000 $0001
    SWEEP 4 $0000 $0001
    SWEEP 5 $0000 $0001
    SWEEP 6 $0000 $0001
    SWEEP 7 $0000 $0001
    SWEEP 8 $0000 $0001
    SWEEP 9 $0000 $0001
    SWEEP 10 $0000 $0001
    SWEEP 11 $0000 $0001
    SWEEP 12 $0000 $0001
    SWEEP 13 $0000 $0001
    SWEEP 14 $0000 $0001
    SWEEP 15 $0000 $0001
    ld l,b
    ld h,c
    ld a,$0C
    call mb_record
    ld a,11
    ld ($0001),a
.ENDIF

.IF FIXTURE_BOARD == 3
    ; Selecting another page in slot 0 moves the code under the CPU, so that
    ; write runs from work RAM; interrupts have been off since reset.
    ld hl,slot0_read
    ld de,STUB
    ld bc,slot0_read_end-slot0_read
    ldir
    ld a,5
    ld ($7FFF),a
    PAIR 2 3 $04
    ld a,3
    ld ($BFFF),a
    PAIR 4 5 $05
    ld a,4
    call STUB
    ld a,$06
    call mb_record
    ld a,8+6
    call STUB
    ld a,$07
    call mb_record
    ld a,8+2
    ld ($7FFF),a
    PAIR 2 3 $08
    ld a,$FF
    ld ($BFFF),a
    PAIR 4 5 $09
    ld hl,$FFFF
    ld (hl),5
    PAIR 4 5 $0A
    ld bc,0
    SWEEP 0 $BFFF $BFFF
    SWEEP 1 $BFFF $BFFF
    SWEEP 2 $BFFF $BFFF
    SWEEP 3 $BFFF $BFFF
    SWEEP 4 $BFFF $BFFF
    SWEEP 5 $BFFF $BFFF
    SWEEP 6 $BFFF $BFFF
    SWEEP 7 $BFFF $BFFF
    ld l,b
    ld h,c
    ld a,$0B
    call mb_record
    ld a,5
    ld ($BFFF),a
.ENDIF

    ; The backdrop colour is the stamp number read through window 5, which
    ; each board has just paged to bank 11: a wrong bank paints another
    ; colour. Registers 0 and 1 turn on the Mode 4 display (SMS-VDP section 2).
    xor a
    out ($BF),a
    ld a,$C0
    out ($BF),a
    ld a,($A000+STAMP)
    ld b,32
paint:
    out ($BE),a
    djnz paint
    ld a,$04
    out ($BF),a
    ld a,$80
    out ($BF),a
    ld a,$40
    out ($BF),a
    ld a,$81
    out ($BF),a

    ; Finish no earlier than the second frame, so a run cut after the first
    ; frame finds the status still running. Status bit 7 is the frame
    ; interrupt flag; reading the status clears it (SMS-VDP section 2.6).
    in a,($BF)
wait_frame:
    in a,($BF)
    or a
    jp p,wait_frame

    call mb_done
done:
    jr done

.IF FIXTURE_BOARD != 3
; Writes 1 to 4 to $FFFC-$FFFF through HL, so they are not store sites: these
; are the Sega registers, which the board lacks.
sega_writes:
    ld hl,$FFFC
    ld (hl),1
    inc hl
    ld (hl),2
    inc hl
    ld (hl),3
    inc hl
    ld (hl),4
    ret
.ENDIF

.IF FIXTURE_BOARD == 3
; A = the page to leave slot 0 on. Returns HL = S(0) | S(1) << 8 and leaves
; slot 0 on page 0. Copied to STUB and run from there; position independent.
slot0_read:
    ld ($3FFE),a
    ld a,(STAMP)
    ld l,a
    ld a,($2000+STAMP)
    ld h,a
    xor a
    ld ($3FFE),a
    ret
slot0_read_end:
.ENDIF

.INCLUDE "mailbox.inc"

    END_STAMP 0

.MACRO BANK_STAMPS ARGS N
.BANK N SLOT 0
    START_STAMP N
    END_STAMP N
.ENDM

    BANK_STAMPS 1
    BANK_STAMPS 2
    BANK_STAMPS 3
    BANK_STAMPS 4
    BANK_STAMPS 5
    BANK_STAMPS 6
    BANK_STAMPS 7
    BANK_STAMPS 8
    BANK_STAMPS 9
    BANK_STAMPS 10
    BANK_STAMPS 11
    BANK_STAMPS 12
    BANK_STAMPS 13
    BANK_STAMPS 14
    BANK_STAMPS 15
