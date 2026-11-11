; Owned Pause NMI fixture (MIT). A Z80 program that counts while it waits, takes
; the frame interrupt, and records how far the count had got and what the V
; counter read when the Pause button's NMI arrived. It judges nothing; the
; records go through the RAM mailbox. Built for 192, 224 and 240 lines
; (LINES_HEIGHT).
;
; Hardware: the console latches a Pause press and the VDP asserts /NMI at tick
; 611 of line lines-2 (260 on NTSC, 311 on PAL, at every height; SMS-SYSTEM
; section 7, SMS-VDP section 4.4). The frame interrupt comes at tick 607 of line
; H, H the screen height (SMS-VDP section 4.3). So the NMI follows the frame
; interrupt by (lines - 2 - H) x 684 + 4 master ticks. The Game Gear in its own
; mode has no Pause NMI. The V counter reads $FF at the NMI, because it stepped
; to the last line's value at tick 604 and steps on only at tick 604 of the next
; line (SMS-VDP section 4.4).
;
; Fault prevented: a Pause NMI asserted on the wrong line or at the wrong time
; on PAL, which only a Z80 program that counts and reads V at the interrupt can
; see end to end (an in-process test proves the tick, not the effect a program
; sees), and an expectation fitted to whatever the core printed. The band the
; count must lie in is computed from the cycle counts below and the tick
; distance above (tests/fixtures/tests.cmake, vtable and pause closed forms),
; never from a run.
;
; Program. IM 1. The frame handler ($38) zeroes the 16-bit count n and counts
; frames; the main loop adds one to n in a fixed body; the NMI handler ($66)
; copies n and V on its first call only and counts its calls.
;
; Lost update. The frame handler zeroes n. A bare load, increment, store of
; the word could be interrupted after the load and the stale value plus one
; stored over the zero; a split 8-bit increment could leave 0100. So the three
; instructions run between DI and EI: the frame interrupt waits until the
; instruction after EI has run, and an NMI, which DI does not mask, only reads
; n, and the word is written by one instruction, so a read never sees half of
; it. No other code writes n.
;
; Cycle counts (documented Z80 T-states):
;   main loop body B = DI 4 + LD HL,(nn) 16 + INC HL 6 + LD (nn),HL 16 + EI 4
;     + LD A,(nn) 13 + CP n 7 + JR C taken 12 = 78
;   frame handler H = EX AF,AF' 4 + IN A,(n) 11 + EXX 4 + LD (nn),DE 20
;     + INC (HL) 11 + EXX 4 + EX AF,AF' 4 + EI 4 + RETI 14 = 76, from the end
;     of the 13-cycle IM 1 accept to the instruction the loop resumes at
; The frame interrupt is taken only between instructions with the flip-flop
; set: after LD A,(nn) (cycle 59 of the body), after CP (66) and after JR (78,
; the start of the next body). The longest wait is 59 cycles from just after
; the JR, at most B. Resuming at 78 the first increment ends 42 cycles later
; (DI to the store), resuming at 66 it ends 54 later, at 59 it ends 61 later.
; So the first increment after the handler ends at least 13 + 76 + 42 = 131 and
; at most 59 + 1 + 13 + 76 + 61 = 210 cycles after the interrupt asserts (the
; 1 is a cycle of tick rounding). The NMI is taken at the end of the
; instruction in progress, at most 16 cycles (LD (nn),HL) plus a cycle of
; rounding after the NMI asserts at C cycles. n counts the increments ended by
; then, so
;   floor((C - S_LO) / B) <= n <= floor((C - S_HI) / B), C = floor(ticks / 3)
;   S_LO = 210 - B = 132     S_HI = 131 - 17 - B = 36
; The NMI handler's own cycles never matter to n: the main loop is stopped
; while it runs. They matter to V: the handler reads V within 120 cycles of the
; assertion, and V steps on 225 cycles after it. One line is 228 cycles, so a
; core that asserts the NMI a line late moves n by about three bodies, outside
; the band, and V reads $00.
;
; Memory: n $C300 (2 bytes), frame count $C302, V at the first NMI $C310, n at
; the first NMI $C311 (2 bytes), NMI count $C314, NMI count when frame 12 ran
; $C315. Records (value 16 bits, low byte first):
;   01  n at the first NMI
;   02  V at the first NMI
;   03  NMI count at frame 12 (a Pause held for several frames gives one NMI)
;   04  NMI count at frame 36 (a second press later gives a second NMI)
; On the native Game Gear no NMI comes and every record is 0.

.INCLUDE "header.inc"
.IFNDEF LINES_HEIGHT
.DEFINE LINES_HEIGHT 192
.ENDIF
.IF LINES_HEIGHT == 224
.DEFINE MODE_BITS $10
.ELSE
.IF LINES_HEIGHT == 240
.DEFINE MODE_BITS $08
.ELSE
.DEFINE MODE_BITS $00
.ENDIF
.ENDIF

.DEFINE N_CNT $C300
.DEFINE FRAMES $C302
.DEFINE V_AT $C310
.DEFINE N_AT $C311
.DEFINE NMI_CNT $C314
.DEFINE NMI_AFTER $C315
.DEFINE FRAME_COPY 12
.DEFINE FRAME_END 36

.BANK 0 SLOT 0
.ORG $0000
    di
    ld sp,$DFF0
    jp main

; The frame interrupt. The shadow registers hold two constants set in main:
; DE' = 0 and HL' = FRAMES, so the handler saves nothing but AF. H = 76.
.ORG $0038
    ex af,af'           ; 4
    in a,($BF)          ; 11  acknowledge the VDP
    exx                 ; 4
    ld (N_CNT),de       ; 20  n = 0
    inc (hl)            ; 11  one more frame
    exx                 ; 4
    ex af,af'           ; 4
    ei                  ; 4
    reti                ; 14

; The Pause NMI. First call only: n and V. Every call: one more NMI.
.ORG $0066
    push af
    push hl
    ld hl,(N_CNT)
    ld a,(NMI_CNT)
    or a
    jr nz,nmi_more
    ld (N_AT),hl
    in a,($7E)          ; V, within 120 cycles of the assertion
    ld (V_AT),a
nmi_more:
    ld hl,NMI_CNT
    inc (hl)
    pop hl
    pop af
    retn

main:
    ld a,8
    call mb_init
    ; The variables start at zero.
    ld hl,0
    ld (N_CNT),hl
    ld (N_AT),hl
    xor a
    ld (FRAMES),a
    ld (V_AT),a
    ld (NMI_CNT),a
    ld (NMI_AFTER),a
    ; The handler's constants.
    exx
    ld de,0
    ld hl,FRAMES
    exx
    ; Mode 4 with the mode-2 bit and the display on, as vdp_vtable, and the
    ; frame interrupt enabled (register 1 bit 5).
    ld a,$06
    out ($BF),a
    ld a,$80
    out ($BF),a
    ld a,$60 | MODE_BITS
    out ($BF),a
    ld a,$81
    out ($BF),a
    in a,($BF)
    im 1
    ei
    ; Until frame 12 has run, then until frame 36 has run: the same body, B = 78.
loop_first:
    di                  ; 4
    ld hl,(N_CNT)       ; 16
    inc hl              ; 6
    ld (N_CNT),hl       ; 16
    ei                  ; 4
    ld a,(FRAMES)       ; 13
    cp FRAME_COPY       ; 7
    jr c,loop_first     ; 12 taken, 7 not
    ld a,(NMI_CNT)
    ld (NMI_AFTER),a
loop_last:
    di                  ; 4
    ld hl,(N_CNT)       ; 16
    inc hl              ; 6
    ld (N_CNT),hl       ; 16
    ei                  ; 4
    ld a,(FRAMES)       ; 13
    cp FRAME_END        ; 7
    jr c,loop_last      ; 12 taken, 7 not
    di
    ld hl,(N_AT)
    ld a,1
    call mb_record
    ld a,(V_AT)
    ld l,a
    ld h,0
    ld a,2
    call mb_record
    ld a,(NMI_AFTER)
    ld l,a
    ld h,0
    ld a,3
    call mb_record
    ld a,(NMI_CNT)
    ld l,a
    ld h,0
    ld a,4
    call mb_record
    call mb_done
done:
    jr done

.INCLUDE "mailbox.inc"
