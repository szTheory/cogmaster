; The Codemasters mapper fixture: a 128 KiB image of 8 banks of 16 KiB. Each
; bank holds a 16-byte stamp at offset $1F00: its number, the number's
; complement and 14 bytes of text. The ROM pages the banks and reports what it
; reads through the RAM mailbox; it judges nothing.
;
; The Codemasters mapper (SMS-CARTRIDGES section 2.2): writes to $0000, $4000
; and $8000 select the bank in slots 0, 1 and 2, the banks are 0, 1, 0 at
; power-on, no part of slot 0 is fixed, and a $4000 write with bit 7 set maps
; 8 KiB of cartridge RAM at $A000-$BFFF. Codemasters games carry no TMR SEGA
; header; their own header sits at $7FE0 (section 1.2) and is written here as
; literal bytes. Its checksum word is not the true sum, which the probe does
; not check.
;
; Records, value low byte first:
;   01  the stamp numbers at reset: slot 0 | slot 1 << 4 | slot 2 << 8
;   02  sum of the stamp words (bank | complement << 8) of banks 0-7 via slot 0
;   03  the same sum via slot 1
;   04  the same sum via slot 2
;   05  slot 2 after $8000 = 35 | after $8000 = $FF << 8
;   06  after $4000 = $81 and writes of $A5 to $A000 and $3C to $BFFF:
;       the byte at $A000 | the byte at $BFFF << 8
;   07  after $4000 = $01: the ROM byte at $A000 | after $4000 = $81 again:
;       the byte at $A000 << 8

.DEFINE FIXTURE_MAP

.MEMORYMAP
DEFAULTSLOT 0
SLOT 0 START $0000 SIZE $4000
SLOT 1 START $4000 SIZE $4000
SLOT 2 START $8000 SIZE $4000
.ENDME

.ROMBANKMAP
BANKSTOTAL 8
BANKSIZE $4000
BANKS 8
.ENDRO

.EMPTYFILL $FF

.DEFINE ROM_ID 2
.DEFINE STAMP $1F00
.DEFINE BANK_COUNT 8
; Where the slot-switching loop runs from: work RAM above the mailbox.
.DEFINE STUB $C200
; Bank 0's byte at $2000, read at $A000 while the cartridge RAM is off.
.DEFINE MARKER $5A

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

    ld a,(STAMP+$4000)
    rlca
    rlca
    rlca
    rlca
    ld b,a
    ld a,(STAMP)
    or b
    ld l,a
    ld a,(STAMP+$8000)
    ld h,a
    ld a,$01
    call mb_record

    ; Selecting another bank in slot 0 moves the code under the CPU, so the
    ; loop runs from work RAM; interrupts have been off since reset.
    ld hl,bank_sum
    ld de,STUB
    ld bc,bank_sum_end-bank_sum
    ldir

    ld ix,STAMP
    ld de,$0000
    xor a
    call sum_slot
    ld a,$02
    call mb_record

    ld ix,STAMP+$4000
    ld de,$4000
    ld a,1
    call sum_slot
    ld a,$03
    call mb_record

    ld ix,STAMP+$8000
    ld de,$8000
    xor a
    call sum_slot
    ld a,$04
    call mb_record

    ld a,35
    ld ($8000),a
    ld a,(STAMP+$8000)
    ld l,a
    ld a,$FF
    ld ($8000),a
    ld a,(STAMP+$8000)
    ld h,a
    xor a
    ld ($8000),a
    ld a,$05
    call mb_record

    ld a,$81
    ld ($4000),a
    ld a,$A5
    ld ($A000),a
    ld a,$3C
    ld ($BFFF),a
    ld a,($A000)
    ld l,a
    ld a,($BFFF)
    ld h,a
    ld a,$06
    call mb_record

    ld a,$01
    ld ($4000),a
    ld a,($A000)
    ld l,a
    ld a,$81
    ld ($4000),a
    ld a,($A000)
    ld h,a
    ld a,$01
    ld ($4000),a
    ld a,$07
    call mb_record

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

; A = the bank to leave the slot on, DE = the slot's bank register, IX = the
; stamp's address in the slot. Returns HL = the sum of the stamp words of
; banks 0-7, modulo $10000.
sum_slot:
    ex af,af'
    jp STUB

; Copied to STUB and run from there; position independent.
bank_sum:
    ld hl,0
    xor a
bank_sum_next:
    ld (de),a
    ld c,(ix+0)
    ld b,(ix+1)
    add hl,bc
    inc a
    cp BANK_COUNT
    jr nz,bank_sum_next
    ex af,af'
    ld (de),a
    ret
bank_sum_end:

.INCLUDE "mailbox.inc"

.ORG $2000
    .DB MARKER

; The Codemasters header (SMS-CARTRIDGES section 1.2): the page count, a BCD
; date (day, month, year 94, hour, minute), a checksum word and its
; complement, then zeros up to $7FEF.
.BANK 1 SLOT 1
.ORG $3FE0
    .DB BANK_COUNT, $20, $10, $94, $12, $00
    .DW $1234, $10000-$1234
    .DB 0, 0, 0, 0, 0, 0

.MACRO STAMP_BANK ARGS N
.BANK N SLOT 0
.ORG STAMP
    .DB N, $FF-N, "cogmaster bank"
.ENDM

    STAMP_BANK 0
    STAMP_BANK 1
    STAMP_BANK 2
    STAMP_BANK 3
    STAMP_BANK 4
    STAMP_BANK 5
    STAMP_BANK 6
    STAMP_BANK 7
