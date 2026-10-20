; The ports fixture: a 32 KiB image that reads the I/O chip's and the VDP's
; ports and reports what it reads through the RAM mailbox; it judges
; nothing. The same image runs under every Master System profile;
; ports_gg.asm includes this file for the Game Gear.
;
; Records, value low byte first:
;   01  $3F = $F5, then $DD & $C0 | $3F = $55, then $DD & $C0 << 8: an
;       output TH reads back its level on an export console only
;       (SMS-SYSTEM sections 4 and 5)

.INCLUDE "header.inc"

.IFDEF FIXTURE_GG
.DEFINE ROM_ID 4
.ELSE
.DEFINE ROM_ID 3
.ENDIF

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
