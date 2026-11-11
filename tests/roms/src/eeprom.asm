; Owned Game Gear EEPROM continuation fixture. It drives the 93C46 on the Sega
; EEPROM board the way a game does: the wiring and the command format are in
; docs/saves.md. Word 0 holds the signature $1234 and word 1 a run counter, so
; the save file starts 34 12 then the counter low byte first.
;
; Each run reads the signature and the counter, enables writes, stores the
; counter plus one, polls Data Out until ready, reads the word back, disables
; writes and tries one more write, which the chip must refuse.
.DEFINE FIXTURE_MAP
.DEFINE FIXTURE_GG
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

.DEFINE ROM_ID 10
.DEFINE EE_WINDOW $8000
.DEFINE EE_CS $04
.DEFINE EE_CS_CLK $06
.DEFINE EE_SIGNATURE $1234
.DEFINE EE_REFUSED $BEEF
.DEFINE EE_EWEN $30
.DEFINE EE_EWDS $00
.DEFINE EE_OP_WRITE $40
.DEFINE EE_OP_READ $80
; Variables in work RAM, below the mailbox.
.DEFINE V_FRESH $C000
.DEFINE V_COUNTER $C001
.DEFINE V_READBACK $C003
.DEFINE V_AFTER $C005
.DEFINE V_READY $C007

.BANK 0 SLOT 0
.ORG 0
    di
    ld sp,$DFF0
    jp main
.ORG $66
    retn
main:
    ld a,ROM_ID
    call mb_init
    ld a,$08                ; open the chip window at $8000
    ld ($FFFC),a
    xor a
    call ee_read            ; word 0: the signature
    ld de,EE_SIGNATURE
    or a
    sbc hl,de
    ld a,h
    or l
    ld (V_FRESH),a          ; 0 when the signature is there
    ld hl,0
    jr nz,counter_ready     ; a fresh chip counts from 0
    ld a,1
    call ee_read            ; word 1: the counter
counter_ready:
    inc hl
    ld (V_COUNTER),hl
    call ee_ewen
    ld a,(V_FRESH)
    or a
    jr z,signed
    xor a
    ld hl,EE_SIGNATURE
    call ee_write
signed:
    ld a,1
    ld hl,(V_COUNTER)
    call ee_write
    ld a,(V_READY)
    ld l,a
    ld h,0
    ld a,4
    call mb_record          ; record 4: ready seen while polling
    ld a,1
    call ee_read
    ld (V_READBACK),hl
    call ee_ewds
    ld a,1
    ld hl,EE_REFUSED
    call ee_write
    ld a,1
    call ee_read
    ld (V_AFTER),hl
    ld hl,(V_COUNTER)
    ld a,1
    call mb_record          ; record 1: the new counter
    ld hl,(V_READBACK)
    ld de,(V_COUNTER)
    call same
    ld a,2
    call mb_record          ; record 2: the word read back is the one written
    ld hl,(V_AFTER)
    ld de,(V_COUNTER)
    call same
    ld a,3
    call mb_record          ; record 3: the write after EWDS changed nothing
    call mb_done
done:
    jr done

; HL = 1 when HL equals DE, else 0.
same:
    or a
    sbc hl,de
    ld hl,0
    ret nz
    inc hl
    ret

; C = data in (0 or 1). One clock pulse with chip select held: the bit is set
; up with the clock low, then the clock rises and the chip takes it.
ee_bit:
    ld a,c
    or EE_CS
    ld (EE_WINDOW),a
    or $02
    ld (EE_WINDOW),a
    ret

; Chip select low: ends a command, and a complete write runs its cycle now.
ee_deselect:
    xor a
    ld (EE_WINDOW),a
    ret

; A = opcode and address byte. Sends the start bit, then the eight bits,
; most significant first.
ee_cmd:
    ld d,a
    ld c,1
    call ee_bit
    ld b,8
ee_cmd_bit:
    ld c,0
    rl d
    rl c
    call ee_bit
    djnz ee_cmd_bit
    ret

; HL = 16 data bits, most significant first.
ee_data:
    ld b,16
ee_data_bit:
    ld c,0
    add hl,hl
    rl c
    call ee_bit
    djnz ee_data_bit
    ret

ee_ewen:
    ld a,EE_EWEN
    call ee_cmd
    jr ee_deselect

ee_ewds:
    ld a,EE_EWDS
    call ee_cmd
    jr ee_deselect

; A = word, HL = value. WRITE, then chip select low starts the cycle; raising
; chip select again shows ready on Data Out, polled up to 256 times.
ee_write:
    or EE_OP_WRITE
    push hl
    call ee_cmd
    pop hl
    call ee_data
    call ee_deselect
    ld a,EE_CS
    ld (EE_WINDOW),a
    ld b,0
ee_poll:
    ld a,(EE_WINDOW)
    rrca
    jr c,ee_ready
    djnz ee_poll
    xor a
    jr ee_polled
ee_ready:
    ld a,1
ee_polled:
    ld (V_READY),a
    jr ee_deselect

; A = word. Returns HL = its value. After the command Data Out shows a dummy
; 0, then each rising clock shows the next bit, bit 15 first.
ee_read:
    or EE_OP_READ
    call ee_cmd
    ld hl,0
    ld b,16
ee_read_bit:
    ld a,EE_CS
    ld (EE_WINDOW),a
    ld a,EE_CS_CLK
    ld (EE_WINDOW),a
    ld a,(EE_WINDOW)
    rrca
    adc hl,hl
    djnz ee_read_bit
    jr ee_deselect
.INCLUDE "mailbox.inc"
