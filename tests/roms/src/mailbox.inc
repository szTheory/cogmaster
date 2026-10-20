; The RAM mailbox a fixture reports through (CONFORMANCE section 4). The
; fixture measures and never judges: tests/diag/expected.csv holds the
; verdicts. Layout, in work RAM away from the stack and the mapper mirror:
;   $C100-$C103  magic "CMDG"
;   $C104        protocol version 1
;   $C105        status: $80 running, $00 done
;   $C106        record count
;   $C107        ROM id
;   $C108-$C109  CRC-16 over $C110-$C1FF, low byte first
;   $C110 + 4n   record n: test id, flags (bit 0 valid), value low, value high
; Include it inside the code of bank 0. The routines change AF, BC, DE, HL.

.DEFINE MB_BASE $C100
.DEFINE MB_STATUS $C105
.DEFINE MB_COUNT $C106
.DEFINE MB_ROM $C107
.DEFINE MB_CRC $C108
.DEFINE MB_RECORDS $C110
.DEFINE MB_RECORDS_LEN 240
.DEFINE MB_RECORDS_MAX 60

; A = ROM id. Clears the mailbox, writes the header and status $80.
mb_init:
    push af
    ld hl,MB_BASE
    ld b,0
    xor a
mb_init_clear:
    ld (hl),a
    inc hl
    djnz mb_init_clear
    ld hl,mb_magic
    ld de,MB_BASE
    ld bc,4
    ldir
    ld a,1
    ld (MB_BASE+4),a
    ld a,$80
    ld (MB_STATUS),a
    pop af
    ld (MB_ROM),a
    ret

mb_magic:
    .DB "CMDG"

; A = test id, HL = value. Appends one valid record; past the 60th, nothing.
mb_record:
    push hl
    ld c,a
    ld a,(MB_COUNT)
    cp MB_RECORDS_MAX
    jr nc,mb_record_full
    ld l,a
    inc a
    ld (MB_COUNT),a
    ld h,0
    add hl,hl
    add hl,hl
    ld de,MB_RECORDS
    add hl,de
    ld (hl),c
    inc hl
    ld (hl),1
    inc hl
    pop de
    ld (hl),e
    inc hl
    ld (hl),d
    ret
mb_record_full:
    pop hl
    ret

; Stores the CRC-16 (CCITT: polynomial $1021, initial value $FFFF, no
; reflection, no final XOR) of the record area, then status $00, last.
mb_done:
    ld hl,MB_RECORDS
    ld bc,MB_RECORDS_LEN
    ld de,$FFFF
mb_crc_byte:
    ld a,(hl)
    xor d
    ld d,a
    push bc
    ld b,8
mb_crc_bit:
    sla e
    rl d
    jr nc,mb_crc_next
    ld a,d
    xor $10
    ld d,a
    ld a,e
    xor $21
    ld e,a
mb_crc_next:
    djnz mb_crc_bit
    pop bc
    inc hl
    dec bc
    ld a,b
    or c
    jr nz,mb_crc_byte
    ld a,e
    ld (MB_CRC),a
    ld a,d
    ld (MB_CRC+1),a
    xor a
    ld (MB_STATUS),a
    ret
