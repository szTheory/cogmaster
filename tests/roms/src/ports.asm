; The ports fixture: a 32 KiB image that reads the I/O chip's and the VDP's
; ports and reports what it reads through the RAM mailbox; it judges
; nothing. The same image runs under every Master System profile;
; ports_gg.asm defines FIXTURE_GG and includes this file for the Game Gear,
; which adds the records of its own ports $00-$06.
;
; Records, value low byte first:
;   01  $3F = $F5, then $DD & $C0 | $3F = $55, then $DD & $C0 << 8: an
;       output TH reads back its level on an export console only
;       (SMS-SYSTEM sections 4 and 5)
;   02  $DD & $30: b4 the reset button (1 where there is none), b5 1 on the
;       Master System and the Game Gear (section 4)
;   03  idle $DC | $DD << 8: every button up (section 4)
;   04  $3E | $C0 << 8: nothing drives a read of $3E; $C0 is $DC on the
;       export console and undriven on full-decode consoles (section 4)
;   05  after $3E b2 = 1: $DC | $DD << 8, the I/O chip off the bus
;   06  after $3E b4 = 1: the byte at $C200 | after b4 = 0 again: the byte
;       at $C200 << 8, with $5A written there first; work RAM off the bus
;   07  the count of a 31-cycle loop polling $BF b7 for one frame
;       (section 5)
;   08  1 when that count is 2,048 or more (a PAL frame), else 0
;   09  the V counter changes from one line-0 reading to the next: one per
;       line (SMS-VDP section 4.4)
;   0A  $A5 and $5A written to VRAM $0000, read back through $BE (SMS-VDP
;       section 2.2)
;   0B  the second of two status reads: the flags clear, bits 4-0 set
;       (SMS-VDP section 2.6)
;   0C  the difference between two $7F reads 15 cycles apart: SMS holds
;       the earlier region-test TH capture; GG has no TH and reads live
;       (SMS-VDP section 4.3, SMS-SYSTEM section 4)
; Game Gear only (SMS-SYSTEM section 8):
;   0D  $00: b7 Start (1 = up), b6 0 = Japan, b5 0 = NTSC
;   0E  $01 | $02 << 8
;   0F  $03 | $04 << 8
;   10  $05 | $06 << 8

.INCLUDE "header.inc"

; $3E as the BIOS leaves it (SMS-SYSTEM section 6), and the bits that take
; the I/O chip and work RAM off the bus (section 4, active low).
.IFDEF FIXTURE_GG
.DEFINE ROM_ID 4
.DEFINE MEM_ON $A8
.ELSE
.DEFINE ROM_ID 3
.DEFINE MEM_ON $AB
.ENDIF
.DEFINE IO_OFF $04
.DEFINE RAM_OFF $10
.DEFINE MARKER $5A
.DEFINE PROBE $C200
; A frame of 2,048 or more 31-cycle loops is a PAL frame (section 5).
.DEFINE PAL_COUNT 2048

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

    ; The region test of Super Tennis and many others (section 5): TH of
    ; both ports as outputs, high, then low; every pin an input again.
    ld a,$F5
    out ($3F),a
    in a,($DD)
    and $C0
    ld l,a
    ld a,$55
    out ($3F),a
    in a,($DD)
    and $C0
    ld h,a
    ld a,$FF
    out ($3F),a
    ld a,$01
    call mb_record

    in a,($DD)
    and $30
    ld l,a
    ld h,0
    ld a,$02
    call mb_record

    in a,($DC)
    ld l,a
    in a,($DD)
    ld h,a
    ld a,$03
    call mb_record

    in a,($3E)
    ld l,a
    in a,($C0)
    ld h,a
    ld a,$04
    call mb_record

    ld a,MEM_ON|IO_OFF
    out ($3E),a
    in a,($DC)
    ld l,a
    in a,($DD)
    ld h,a
    ld a,MEM_ON
    out ($3E),a
    ld a,$05
    call mb_record

    ; Nothing touches the stack while work RAM is off.
    ld a,MARKER
    ld (PROBE),a
    ld a,MEM_ON|RAM_OFF
    out ($3E),a
    ld a,(PROBE)
    ld l,a
    ld a,MEM_ON
    out ($3E),a
    ld a,(PROBE)
    ld h,a
    ld a,$06
    call mb_record

    ; World Soccer's TV test (section 5): from one frame flag to the next,
    ; INC HL (6) + IN A,($BF) (11) + OR A (4) + JP P (10) = 31 cycles.
    in a,($BF)
tv_sync:
    in a,($BF)
    or a
    jp p,tv_sync
    ld hl,0
tv_count:
    inc hl
    in a,($BF)
    or a
    jp p,tv_count
    push hl
    ld a,$07
    call mb_record
    ; The flag rose a few instructions ago, so no flag rises between
    ; these two status reads.
    in a,($BF)
    in a,($BF)
    ld l,a
    ld h,0
    ld a,$0B
    call mb_record
    pop hl
    ld de,PAL_COUNT
    or a
    sbc hl,de
    ld hl,0
    jr c,tv_ntsc
    inc l
tv_ntsc:
    ld a,$08
    call mb_record

    ; Wait for a V counter of 0 after one that is not, then count its
    ; changes until it reads 0 again; a sample is 27 cycles, well inside
    ; a 228-cycle line.
vc_leave:
    in a,($7E)
    or a
    jr z,vc_leave
vc_enter:
    in a,($7E)
    or a
    jr nz,vc_enter
    ld b,a
    ld hl,0
vc_count:
    in a,($7E)
    cp b
    jr z,vc_count
    ld b,a
    inc hl
    or a
    jr nz,vc_count
    ; Line 0 has just begun: GG's live reads are far from the H jump;
    ; SMS still holds the earlier region-test capture and reports zero.
    in a,($7F)
    ld b,a
    in a,($7F)
    sub b
    ld c,a
    push bc
    ld a,$09
    call mb_record
    pop bc
    ld l,c
    ld h,0
    ld a,$0C
    call mb_record

    ; The display is off (R1 = 0), so the VDP takes every access at once
    ; (SMS-VDP section 2.2): write address $0000, two bytes, then read
    ; address $0000 and read them back.
    xor a
    out ($BF),a
    ld a,$40
    out ($BF),a
    ld a,$A5
    out ($BE),a
    ld a,$5A
    out ($BE),a
    xor a
    out ($BF),a
    out ($BF),a
    in a,($BE)
    ld l,a
    in a,($BE)
    ld h,a
    ld a,$0A
    call mb_record

.IFDEF FIXTURE_GG
    in a,($00)
    ld l,a
    ld h,0
    ld a,$0D
    call mb_record
    in a,($01)
    ld l,a
    in a,($02)
    ld h,a
    ld a,$0E
    call mb_record
    in a,($03)
    ld l,a
    in a,($04)
    ld h,a
    ld a,$0F
    call mb_record
    in a,($05)
    ld l,a
    in a,($06)
    ld h,a
    ld a,$10
    call mb_record
.ENDIF

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

.INCLUDE "mailbox.inc"
