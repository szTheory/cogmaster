; The Sega mapper fixture: a 512 KiB image of 32 banks of 16 KiB. Each bank
; holds a 16-byte stamp at offset $0400: its number, the number's complement
; and 14 bytes of text. The ROM pages the banks and reports what it reads
; through the RAM mailbox; it judges nothing.
;
; The Sega mapper (SMS-CARTRIDGES section 2.1): writes to $FFFD, $FFFE and
; $FFFF select the bank in slots 0, 1 and 2, and slot 0's first 1 KiB stays
; on bank 0. Every line of code is below $0400, so it never moves.
;
; Records, value low byte first:
;   01  sum of the stamp words (bank | complement << 8) of banks 0-31 via slot 1
;   02  the same sum via slot 2
;   03  after $FFFD = 5: the byte at $0000 | the byte at $03FF << 8
;   04  after $FFFD = 5: the stamp word at $0400

.DEFINE FIXTURE_MAP

.MEMORYMAP
DEFAULTSLOT 0
SLOT 0 START $0000 SIZE $4000
SLOT 1 START $4000 SIZE $4000
SLOT 2 START $8000 SIZE $4000
.ENDME

.ROMBANKMAP
BANKSTOTAL 32
BANKSIZE $4000
BANKS 32
.ENDRO

.EMPTYFILL $FF

.INCLUDE "header.inc"

.DEFINE ROM_ID 1
.DEFINE STAMP $0400
.DEFINE BANK_COUNT 32
; Bank 0's last byte below the fixed 1 KiB, read back after slot 0 moves.
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

    ld hl,$4000+STAMP
    ld de,$FFFE
    call bank_sum
    ld a,$01
    call mb_record
    ld a,1
    ld ($FFFE),a

    ld hl,$8000+STAMP
    ld de,$FFFF
    call bank_sum
    ld a,$02
    call mb_record
    ld a,2
    ld ($FFFF),a

    ld a,5
    ld ($FFFD),a
    ld a,($0000)
    ld l,a
    ld a,(STAMP-1)
    ld h,a
    ld a,(STAMP)
    ld e,a
    ld a,(STAMP+1)
    ld d,a
    xor a
    ld ($FFFD),a
    push de
    ld a,$03
    call mb_record
    pop hl
    ld a,$04
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

; HL = the stamp's address in a slot, DE = that slot's bank register.
; Returns HL = the sum of the stamp words of banks 0-31, modulo $10000.
bank_sum:
    push hl
    pop ix
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
    ret

.INCLUDE "mailbox.inc"

.ORG STAMP-1
    .DB MARKER

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
    STAMP_BANK 8
    STAMP_BANK 9
    STAMP_BANK 10
    STAMP_BANK 11
    STAMP_BANK 12
    STAMP_BANK 13
    STAMP_BANK 14
    STAMP_BANK 15
    STAMP_BANK 16
    STAMP_BANK 17
    STAMP_BANK 18
    STAMP_BANK 19
    STAMP_BANK 20
    STAMP_BANK 21
    STAMP_BANK 22
    STAMP_BANK 23
    STAMP_BANK 24
    STAMP_BANK 25
    STAMP_BANK 26
    STAMP_BANK 27
    STAMP_BANK 28
    STAMP_BANK 29
    STAMP_BANK 30
    STAMP_BANK 31
