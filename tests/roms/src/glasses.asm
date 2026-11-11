; The 3-D glasses fixture: a 32 KiB image that drives the shutter register the
; way a 3-D title does, one write a frame, and reports through the RAM mailbox
; that it ran; it judges nothing. The runner's --eyes counts are judged against
; the table below. Plain assembly written from SMS-CARTRIDGES-AND-PERIPHERALS
; section 3.4 and SMS-SYSTEM section 3.
;
; A write to $FFF8 (it mirrors at $FFF8-$FFFB) sets the shutter: bit 0 clear
; opens the right eye, set the left. The register is write-only; the byte
; lands in work RAM's mirror at $DFF8, the only way to read it back. The
; adapter snoops the bus, so it sees the write whatever $3E says about the
; cartridge slot.
;
; Each frame the program writes once right after the frame wait, early in the
; frame, or skips. The table holds a byte a frame: 0 or 1 is the byte written,
; $FF is no write. Frames 1-2 write nothing; 3-10 alternate the eye, right
; first; frame 11 writes once and frames 12-16 write nothing (a title that sets
; one eye and keeps it); 17-24 alternate again from a 16-byte routine copied to
; work RAM, with $3E bit 6 set so the cartridge is off the bus while it runs.
; Frame 25 on writes nothing; the mailbox work lasts a few more frames.
;
; Records, value low byte first:
;   01  the length of the RAM routine in bytes
;   02  the byte the work RAM mirror holds after the routine: the last byte it
;       wrote, 7 after eight writes counting up from 0, whose bit 0 is the eye

.INCLUDE "header.inc"

.DEFINE ROM_ID 13
.DEFINE SHUTTER $FFF8
.DEFINE MIRROR $DFF8
.DEFINE RAM_CODE $C300
.DEFINE SKIP $FF
.DEFINE ROM_FRAMES 16
.DEFINE RAM_FRAMES 8
; $3E as the console leaves it, and with the cartridge slot off (b6).
.DEFINE MEM_NORMAL $AB
.DEFINE MEM_CART_OFF $EB

.BANK 0 SLOT 0
.ORG $0000
    di
    ld sp,$DFF0
    jp main
.ORG $0066
    retn

main:
    ld a,ROM_ID
    call mb_init

    ; Frames 1-16 from the table. The first entry is used at once, in frame
    ; 1; each later one after the frame flag, so entry n is written in frame n.
    ld hl,eye_table
    ld b,ROM_FRAMES
rom_loop:
    ld a,(hl)
    inc hl
    cp SKIP
    jr z,rom_skip
    ld (SHUTTER),a
rom_skip:
    call wait_frame
    djnz rom_loop

    ; Frames 17-24 from RAM with the cartridge off: C is the eye, B the frames.
    ld hl,ram_routine
    ld de,RAM_CODE
    ld bc,ram_end - ram_routine
    ldir
    ld a,(eye_table + ROM_FRAMES)
    ld c,a
    ld b,RAM_FRAMES
    ld hl,SHUTTER
    ld a,MEM_CART_OFF
    call RAM_CODE

    ld hl,ram_end - ram_routine
    ld a,$01
    call mb_record
    ld a,(MIRROR)
    ld l,a
    ld h,0
    ld a,$02
    call mb_record
    call mb_done
idle:
    call wait_frame
    jr idle

eye_table:
    .DB $FF,$FF, 0,1,0,1,0,1,0,1, 0,$FF,$FF,$FF,$FF,$FF, 0,1,0,1,0,1,0,1

; Runs from work RAM with the cartridge off the bus. A is $3E with b6 set and
; HL is $FFF8. It writes C and counts it up, so bit 0 flips, then waits for
; the frame flag (status bit 7), once a frame, and puts $3E back before it
; returns; only bit 0 of the written byte is the eye. No absolute address.
ram_routine:
    out ($3E),a
ram_loop:
    ld (hl),c
    inc c
ram_wait:
    in a,($BF)
    rla
    jr nc,ram_wait
    djnz ram_loop
    ld a,MEM_NORMAL
    out ($3E),a
    ret
ram_end:

; Status bit 7 is the frame interrupt flag; reading the status clears it
; (SMS-VDP section 2.6).
wait_frame:
    in a,($BF)
    or a
    jp p,wait_frame
    ret

.INCLUDE "mailbox.inc"
