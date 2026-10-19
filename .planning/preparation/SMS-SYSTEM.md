---
title: "SMS and Game Gear system: clocks, line timeline, Z80 interrupts, memory map, I/O ports, regions, BIOS and boot"
summary: "Tick-level system facts with sources and disagreements: the crystals and the one-VDP-clock tick, where interrupts and counter reads land in a line, Z80 acceptance rules and the absence of wait states, the decoded memory and I/O maps per machine, region and TV detection, the BIOS family with public digests, the fast-boot hand-off state and the power-on profile fields."
read_when: "Before designing or changing the time base, bus, CPU wrapper, I/O decode, machine profiles, power-on state, boot/fast-boot path or any timing that crosses chips."
updated: 2026-10-18
---

# SMS and Game Gear system timing, maps and boot

| Question | Section |
|---|---|
| What is one tick, and how many per Z80 cycle, pixel, line and frame, per region and on Game Gear? | 1 |
| Where in a line do the frame interrupt, line interrupt, NMI and counter reads land; how does the Z80 accept them; are there wait states? | 2 |
| What answers each address, how do the paging registers work, what does an empty read return? | 3 |
| Which port is which on SMS1, SMS2, Japanese SMS, Mark III and Game Gear, and what do $3E, $3F, $DC, $DD, $F2 and GG $00 to $06 hold? | 4 |
| Which machine variants exist, how games detect region and TV type, which profile fields the core needs | 5 |
| Which BIOSes exist, what they check, what the fast-boot hand-off must reproduce | 6 |
| Pause NMI, the reset button, what a reset keeps | 7 |
| Every power-on value the hardware leaves undefined, with a recommendation | 8 |
| Unknowns and the phase that settles each | 9 |

Coordinates: a line is 684 ticks; tick 0 = first active pixel (pixel 0), the origin of Charles MacDonald's measurements
[SYS.11]. Pixel p = ticks 2p and 2p+1. Units are ticks unless stated. Pixel-accurate VDP behaviour lives in SMS-VDP.md.

## 1. Clocks and the master tick

| | NTSC SMS | PAL SMS | Game Gear | Basis |
|---|---|---|---|---|
| Crystal | 53,693,175 Hz (53.6931 MHz) | 53,203,424 Hz (53.2034 MHz; MAME uses 53,203,425) | 32,215,905 Hz | [SYS.01][SYS.25] |
| VDP clock = **1 tick** | crystal/5 = 10,738,635 Hz | crystal/5 = 10,640,684.8 Hz | crystal/3 = 10,738,635 Hz | [SYS.25]; MAME feeds the SMS VDP a 10,738,635 Hz XTAL and the GG VDP crystal/3 |
| Z80 (and PSG input) | 3 ticks = 3,579,545 Hz | 3 ticks = 3,546,894.93 Hz (SMS Power! quotes 3,546,893, "true" 3,546,895) | 3 ticks = 3,579,545 Hz | [SYS.01][SYS.25]: Z80 = crystal/15, GG crystal/9 |
| Pixel | 2 ticks = 5,369,317.5 Hz | 2 ticks | 2 ticks | [SYS.11][SYS.25] |
| Line | 342 pixels = 684 ticks = 228 Z80 cycles = 63.70 µs | 684 ticks = 64.28 µs | 684 ticks | [SYS.10][SYS.11][SYS.29] |
| Frame | 262 lines = 179,208 ticks = 59,736 cycles | 313 lines = 214,092 ticks = 71,364 cycles | 262 lines | [SYS.10][SYS.11] |
| Line rate | 10,738,635/684 = 15,699.76 Hz | 15,556.56 Hz | 15,699.76 Hz | derived |
| Frame rate | 3,579,545/59,736 = 59.922743 Hz | 13,300,856/267,615 = 49.701459 Hz | 59.922743 Hz | derived; Charles measured ≈ 59.92 Hz with mclk 10.738580 MHz [SYS.11] |
| Brazil PAL-M, Argentina PAL-N | VDP 10,726,833 Hz (nominal 10,726,834) and 10,746,168 Hz, NTSC line count | | | [SYS.25]; PAL-M counts as NTSC for TV detection [SYS.06] |

- The Z80 cycle grid never drifts against the line: 684 = 3 × 228, and a frame is a whole number of cycles in both regions, so the phase between CPU and VDP is fixed for ever from power-on (derived). This is the biggest timing simplification relative to the 7800 sibling.
- The PSG divides the Z80 clock by 16 (48 ticks per PSG step); the YM2413 runs at the Z80 clock [SYS.25]. Audio detail is in SMS-AUDIO.md.

**C1 Tick unit.** Recommendation: one `uint64_t` counter of VDP clocks (crystal/5, GG crystal/3). Z80 cycle = 3, pixel = 2,
PSG step = 48, line = 684, every measured event in [SYS.11] is already in this unit, and NTSC SMS and Game Gear share
one tick rate. Strongest alternative: crystal periods (×5), as a "true master". Would change if: any device were found
clocked at crystal/1 or crystal/2 (none is: the VDP's internal fastest clock is crystal/5 [SYS.25]).
**C2 PAL rate.** Recommendation: 53,203,424 Hz for reported rates (SMS Power! [SYS.01]); tick behaviour is identical.
Strongest alternative: MAME's 53,203,425 [SYS.25]. Would change if: a frequency-counter reading on a PAL board.

## 2. The line as the CPU sees it

| Tick | Z80 cycle | Event | Sources |
|---|---|---|---|
| 0–511 | 0–170 | 256 active pixels | [SYS.11] |
| 512–683 | 171–227 | 172 ticks of border, blanking, sync and burst: right border 15 px, right blank 8, HSYNC 26, left blank 2, burst 14, left blank 8, left border 13 | [SYS.10][SYS.27] |
| 558 | 186 | /HSYNC falls, 53 ticks wide | [SYS.11] |
| 588 | 196 | H counter "jump backwards" per Sega's documentation (unconfirmed on a scope) | [SYS.11] |
| ≥ 599 | ≥ 200 | an `IN A,($7E)` sees the next line's V counter value (12 ticks before the /CSYNC rising edge) | [SYS.11] |
| 607 | 202 | frame /INT falls on the first line after active display (line 192, 224 or 240), 4 ticks before /HSYNC rises | [SYS.11]; flag line V = $C1/$E1/$F1 [SYS.10] |
| 608 | 203 | line /INT falls on the line whose counter underflowed, 3 ticks before /HSYNC rises | [SYS.11] |
| 611 | 204 | /HSYNC rises; on the last frame line (261 NTSC, 312 PAL) the pause /NMI is asserted here, whenever the button was pressed earlier | [SYS.11] |

- A frame interrupt and a line interrupt can never fall on the same Z80 cycle; the frame one leads by one cycle [SYS.11].
- Line counter rule: reloaded from register $0A on every line outside the active area except the one right after it, decremented
  on active lines and that one line; underflow sets the pending flag and reloads [SYS.10][SYS.29]. ares raises the frame
  flag at tick 0 of line 193 (one line later and 77 ticks earlier than measured) [SYS.29]; MAME places vint at its pixel 24 and
  the V counter change at pixel 23 in its own origin, i.e. counter first, interrupt just after, the same order as [SYS.11] [SYS.27].
  Christophe and FluBBa estimate the interrupt nearer H counter $F4–$F6 (≈ 654 ticks), unresolved against Charles's 607 [SYS.11].
- V counter read sequences (hex): NTSC 192: 00–DA, D5–FF; NTSC 224: 00–EA, E5–FF; NTSC 240: 00–FF, 00–06 (no valid picture);
  PAL 192: 00–F2, BA–FF; PAL 224: 00–FF, 00–02, CA–FF; PAL 240: 00–FF, 00–0A, D2–FF [SYS.10][SYS.17]; ares implements them as a
  subtraction above a threshold [SYS.29].
- H counter: 9 bits, reads return the upper 8; latched into the read value when TH on either port changes [SYS.10]; MAME and ares
  latch on a low-to-high TH transition [SYS.26][SYS.28]; the live counter is otherwise never readable [SYS.10].

**Z80 model needed for per-cycle vectors.** M1 = 4 T-states (opcode sampled at the T3 rising edge, refresh on T3–T4), memory
read/write 3, I/O 4 with one automatic wait state, interrupt acknowledge M1 with two automatic wait states; /WAIT is sampled on the
falling edge of T2 and of every wait state [SYS.23]. R's low 7 bits step once per M1 [SYS.23]. The core steps the Z80 by whole
M-cycles: every bus access is time-stamped at its own T-state, which is what the VDP data-port and counter reads need (C3).
**Interrupt acceptance.** /INT is level-sensitive and sampled at the rising edge of the last T-state of every instruction; it is taken
only with IFF1 set, and EI defers acceptance until after the following instruction; the VDP holds /INT low until the status port is
read, so a handler that never reads $BF re-enters at once [SYS.23][SYS.14]. /NMI is a negative edge sampled at the same point, costs 11
T-states, pushes PC, jumps to $0066, copies IFF1 to IFF2 and clears IFF1; RETN restores it [SYS.23]. IM 1 is a RST $38 plus two wait
states: 13 T-states [SYS.23]. Games use IM 1; IM 0 and IM 2 read the data bus, which is $FF on SMS2 and GG and "random" (the last
instruction byte, see C5) on SMS1 [SYS.09]. HALT executes NOPs and leaves on the next rising edge after an interrupt is sampled in T4
[SYS.23]. Reset: PC, I, R and both IFFs clear, IM 0, /RESET held three clocks [SYS.23]; AF = SP = $FFFF per Sean Young (unverified,
not fetched) [SYS.24]. No interrupt after a DD/FD prefix byte (unverified) [SYS.24].
**Wait states and data-port timing.** No source shows anything on the SMS or Game Gear board driving /WAIT: Charles's hardware notes
never mention it, MAME and ares model none [SYS.09][SYS.26][SYS.28]. The VDP does not stall the CPU; a data-port write that arrives
while it is still busy with the previous CRAM write is lost [SYS.10], and MAME lists "VRAM/CRAM access constraints" as unimplemented
[SYS.27]. So the "space VRAM writes by about 26–29 cycles during active display" rule is a software rule from Sega's programming
guidance (figure unverified); the hardware drops or corrupts, it never waits. SMS-VDP.md owns the access-slot model.
**C3 CPU–VDP interface.** Recommendation: the Z80 calls the bus with (address, tick of the data T-state); the VDP catches up to that tick
before serving. Strongest alternative: run the VDP a line at a time and apply counter corrections (ares' per-clock thread is the other
extreme [SYS.29]). Would change if: catch-up proves too slow for the handheld budget; then catch-up only on VDP and counter ports.

## 3. Memory map

| Range | Device | Notes | Sources |
|---|---|---|---|
| $0000–$03FF | slot area, unpaged: the first 1 KB of ROM bank 0 under the Sega mapper (vectors stay put); GG BIOS sits here while enabled | [SYS.02][SYS.03][SYS.04] |
| $0400–$3FFF | slot 0, register $FFFD | shows the last 15 KB of the selected bank | [SYS.02][SYS.03] |
| $4000–$7FFF | slot 1, register $FFFE | | [SYS.02] |
| $8000–$BFFF | slot 2, register $FFFF, or on-cart RAM when $FFFC bit 3 = 1 | | [SYS.02][SYS.03] |
| $C000–$DFFF | 8 KB work RAM (one NEC D4168), selected by A14 = A15 = 1 | disabled by $3E bit 4 | [SYS.20] |
| $E000–$FFFF | mirror of work RAM | writes to $FFF8–$FFFF also reach the device; reads return RAM, which is how software reads back the write-only mapper | [SYS.02][SYS.09] |
| $FFF8 (–$FFFB) | SegaScope 3-D shutter, bit 0; SMS1 only; works with the card slot disabled | | [SYS.09][SYS.26] |
| $FFFC | RAM/misc: bit 7 ROM write enable, 4 RAM at $C000–$FFFF (needs $3E bit 4 set; unused by software), 3 RAM in slot 2, 2 RAM bank, 1–0 bank shift ($00/$18/$10/$08; one 512 KB chip only) | | [SYS.03] |
| $FFFD–$FFFF | bank for slots 0–2; 3, 5 or 6 significant bits by mapper revision; power-of-two ROMs mirror | | [SYS.03] |

- Slot area arbitration is port $3E: expansion, cartridge, card and BIOS each have an active-low enable; the BIOS copies itself to RAM,
  disables its ROM and enables card, cartridge and expansion in turn [SYS.04]. On SMS2 the BIOS and cartridge outputs are ANDed when both
  are enabled [SYS.26]; MAME starts from $FF and ANDs every enabled source, ares applies sources in order with the cartridge overriding
  the BIOS [SYS.26][SYS.28]. Mark III has no $3E: the cartridge bridges the card's /OE, so cartridge beats card [SYS.04][SYS.26].
- Game Gear: with $3E bit 3 clear the 1 KB BIOS answers $0000–$03FF and the cartridge the rest; with it set the cartridge fills the
  first 1 KB too [SYS.04][SYS.09]. MAME notes $3E "only exists in Master System mode" on GG [SYS.25].
- On-cart RAM is 16 KB per bank (two banks, bit 2); most games fit 8 KB mirrored across the slot [SYS.03]. Mapper families (Codemasters
  at $0000/$4000/$8000 with no protected first KB, Korean $A000, and the rest) are in SMS-CARTRIDGES-AND-PERIPHERALS.md [SYS.03].
- Mapper power-on: undefined on the 831001/832011 mask ROMs and the 315-5208; the 315-5235 resets to $FFFC = $00, $FFFD = $00,
  $FFFE = $01, $FFFF = $02 [SYS.03]. BIOS v1.3 never writes the paging registers, so every cartridge must boot with banks 0/1/2 [SYS.08].

**C4 Mapper reset.** Recommendation: 0, 0, 1, 2 for every Sega-mapper board (the 315-5235 values, and what v1.3 relies on) [SYS.03][SYS.08].
Strongest alternative: leave undefined (seeded) for the older chips. Would change if: a title depends on a non-sequential power-on bank.
**C5 Open bus.** SMS1 reads of an unmapped address or port return the last byte of the instruction that performed the read (the Z80's own
operand still on the bus); SMS2 and GG return $FF; Mega Drive $00 or noise [SYS.09]. Reads of unused ports $00–$3F on SMS1 are "open bus",
$FF on SMS2 [SYS.12]. ares keeps one `mdr` byte updated by every access with a per-model pull-up ($FF on SMS2 and GG) [SYS.28]; MAME falls
back to the CPU region image [SYS.26]. Recommendation: one data-bus byte written by every fetch, read and write; an undriven read returns it
on SMS1 and Mark III, and $FF (pull-up) on SMS2 and GG; the bit-level truth is a profile field. Strongest alternative: $FF everywhere.
Would change if: never for SMS1; a region test that distinguishes the two is the first conformance vector to write.

## 4. I/O ports

| Port (SMS1/SMS2 decode: A7, A6, A0 only) | Read | Write | Sources |
|---|---|---|---|
| $00–$3F even ($3E) | SMS1 open bus; SMS2/GG $FF | memory control | [SYS.09][SYS.12] |
| $00–$3F odd ($3F) | same | I/O control | [SYS.09][SYS.12] |
| $40–$7F even ($7E) | V counter | PSG | [SYS.09][SYS.12] |
| $40–$7F odd ($7F) | H counter | PSG | [SYS.09][SYS.12] |
| $80–$BF even ($BE) | VDP data | VDP data | [SYS.09] |
| $80–$BF odd ($BF) | VDP status | VDP control | [SYS.09] |
| $C0–$FF even ($DC, $DE, $C0) | port A/B | none | [SYS.09][SYS.28] |
| $C0–$FF odd ($DD, $DF, $C1) | port B/misc | none | [SYS.09][SYS.28] |
| $F0, $F1 (FM machines) | $DC/$DD mirror | YM2413 register, data | [SYS.12][SYS.19] |
| $F2 (FM machines) | audio control | audio control | [SYS.19] |

- Full decode exceptions: on the Japanese SMS, Korean units, Mark III and Game Gear the $3E/$3F mirrors across $00–$3F do not exist
  (MAME, where keeping them breaks the Korean cartridge Bubble Bobble) [SYS.25]; Mark III has no $3E/$3F at all [SYS.25]. ares decodes
  with mask $C1 everywhere and calls full decode "unconfirmed" [SYS.28].
- $DE/$DF: on SMS they are mirrors of $DC/$DD; they are the SG-1000/SC-3000 8255 ports, and some software probes a keyboard there [SYS.09].
- **$3E memory control** (active low: 0 enables): bit 7 expansion, 6 cartridge, 5 card, 4 work RAM, 3 BIOS ROM, 2 I/O chip, 1–0 unknown.
  With the I/O chip disabled, $C0–$FF read open bus on SMS1 and $FF on SMS2/GG [SYS.09][SYS.04]. Bits 7, 5 and 2 do nothing on SMS2 and GG [SYS.04].
  The BIOS keeps its last $3E value at $C000; games OR $04 into it to disable I/O while probing the FM chip [SYS.08][SYS.18].
- **$3F I/O control**: bits 0–3 direction of port A TR, A TH, B TR, B TH (1 = input); bits 4–7 their output levels [SYS.09][SYS.28].
  Export units read an output TH back on $DD bits 6/7; a Japanese unit does not (MAME returns 0 for an output TH; ares ties the level to
  the direction) [SYS.26][SYS.28]. A low-to-high TH edge latches the H counter [SYS.26][SYS.28]. MAME resets $3F to $FF [SYS.26].
- **$DC**: bits 0–5 port A up, down, left, right, TL, TR; 6–7 port B up, down; 0 = pressed [SYS.09]. **$DD**: 0–3 port B left, right,
  TL, TR; 4 reset button (0 = pressed, 1 where absent); 5 reads 1 on SMS/GG and 0 on Mega Drive; 6–7 TH of A and B [SYS.09][SYS.26].
- **$F2 audio control** (Japanese SMS and Mark III + FM Unit): write bits 1–0 = %00 PSG only (power-on), %01 FM only, %10 both off,
  %11 both; Mark III forces bit 1 to 0; read returns the mute bits in 1–0, zeros in 2–4, and bits 3, 7 and 11 of a 12-bit C-SYNC line
  counter in 5–7 [SYS.19][SYS.26][SYS.28]. Without FM the port is a $DD mirror on SMS ("depends") and reads port 0 on Mark III
  [SYS.12][SYS.19]. Phantasy Star's detector writes 1,0,1,0,1,0,1 and needs bits 0–2 to echo every time [SYS.18].
- **Game Gear $00–$06** (GG mode; in SMS mode they become $3E/$3F mirrors and read $FF) [SYS.13][SYS.28]: $00 read: bit 7 Start
  (0 = pressed), 6 region (0 = Japan), 5 NTSC/PAL flag (MAME header, not wired), 0–4 zero [SYS.26][SYS.28]; $01 parallel link data
  (bit 7 always data, others data or direction); $02 direction bits 0–6 and NMI enable bit 7; $03 TX; $04 RX; $05 status: 0 TX full,
  1 RX full, 2 framing error, 6–7 baud [SYS.28]; $06 stereo: bits 0–3 route PSG channels 0–3 right, 4–7 left, headphones only [SYS.22][SYS.09].
  Power-on image per Charles: $C0 $7F $FF $00 $FF $00 $FF [SYS.09]. GG $DD reads bits 4–6 as 1 [SYS.26][SYS.28].

## 5. Regions, variants and detection

| Machine | VDP | $3E/$3F | Reset button | FM | Pause | Open bus | Notes | Sources |
|---|---|---|---|---|---|---|---|---|
| Mark III (JP 1985) | 315-5124 | none | no | FM Unit add-on, $F2 bit 1 forced 0 | NMI | last instruction byte | TH not connected; cartridge over card | [SYS.25][SYS.19][SYS.16] |
| Japanese SMS (1987) | 315-5124 | full decode, no mirrors | no (rapid-fire switch) | built in | NMI | last instruction byte | BIOS v2.1; MAME fills RAM with $F0 | [SYS.25][SYS.26][SYS.08] |
| Export SMS1 | 315-5124 | A7/A6/A0 mirrors | yes, $DD bit 4 | no | NMI | last instruction byte | card slot, expansion, SegaScope | [SYS.09][SYS.16] |
| Export SMS2 | 315-5246 (224/240 modes) | mirrors | no | no | NMI | $FF | no card/expansion; Alex Kidd or Sonic BIOS | [SYS.07][SYS.16][SYS.08] |
| Game Gear (1990) | SMS2-class with GG mode (MAME 315-5377) | $3E in SMS mode only | no | no | Start → NMI in SMS mode only | $FF | NTSC only; 1 KB BIOS from VA1 | [SYS.21][SYS.25][SYS.26][SYS.30] |
| Mega Drive + PBC | 315-5313 | | no | no | NMI | $00/noise | not emulated; precedent only | [SYS.09] |

- Game Gear SMS mode is selected by the cartridge (the Master Gear converter): the VDP keeps a 6-bit palette and full 256-wide output,
  ports $00–$06 fall back to $3E/$3F mirrors, and the Start button becomes the pause NMI [SYS.21][SYS.13][SYS.26][SYS.30]. MAME picks
  the mode from the cartridge at reset [SYS.26]. The GG BIOS writes and reads port $02 to tell the modes apart [SYS.08].
- Region detection (Super Tennis and many others): write %11110101 to $3F, read $DD, expect bits 7–6 = %11; write %01010101, expect %00;
  any mismatch means Japanese; restore $FF [SYS.05]. Game Gear: $00 bit 6 = 0 means Japanese [SYS.05]. Pop Breaker's mistaken test
  of the whole byte locks export units out (ECO file).
- TV type: count V counter values (262 vs 313), or time a frame with a 31-cycle loop: ≈ 1924 iterations NTSC, ≈ 2288 PAL, threshold
  2048 (World Soccer) [SYS.06]. Japanese + PAL does not exist; export + NTSC = USA/Brazil, export + PAL = Europe/Australia [SYS.05].
- MAME marks the Korean SMS2 as export region and Korean SMS1 clones as Japanese, with the Gam*Boy BIOS lacking FM [SYS.25][SYS.08].

**C6 Profile fields.** Recommendation: `machine` {mark3, sms1, sms2, gg}, `region` {japan, export}, `tv` {ntsc, pal}, `fm` {none, unit,
builtin}, `vdp` {5124, 5246, gg}, `io_decode` {partial, full}, `open_bus` {last_byte, ff}, `reset_button`, `ram_fill`, `bios` (bytes or
none), plus the C8/C9 power-on values. `tv` = pal on a gg or a japan machine is rejected at load. Strongest alternative: a flat list of
named presets only. Would change if: a user-facing preset proves enough; then the fields stay internal.

## 6. BIOS

| BIOS | Size | Built-in game | Boot checks | MAME file, CRC32, SHA-1 | Sources |
|---|---|---|---|---|---|
| Prototype v1.0 (1986) | 8 KB | none | no Snail Maze; boot wait depends on ROM size | `v1.0.bin` 72bec693 29091ff60ef4c22b1ee17aa21e0e75bac6b36474 | [SYS.08][SYS.25] |
| US/EU v1.3 (1986) | 8 KB | Snail Maze (Up+1+2) | header, TMR SEGA, region nibble 4, checksum by size nibble; does not write paging | `mpr-10052.rom` 0072ed54 c315672807d8ddb8d91443729405c766dd95cae7 | [SYS.08][SYS.25] |
| v2.0 (1987, rare) | 8 KB | Snail Maze | only "first 513 bytes not identical" | `mpr-10883.rom` b3d854f8 fc7eb9141f38c92bf98d9134816f64b45e811112 | [SYS.08][SYS.25] |
| Japanese v2.1 (1987) | 8 KB | none (Space Harrier theme) | data-variation check, no header; also Samsung Gam*Boy | `mpr-11124.ic2` 48d44a13 a8c1b39a2e41137835eda6a5de6d46dd9fadbaf2 | [SYS.08][SYS.25] |
| v2.4 Hang On + Safari Hunt (1988) | 128 KB | yes | $2 checksum-range bug; saves/restores bank | `mpr-11459a.rom` 91e93385 9e179392cd416af14024d8f31c981d9ee9a64517 | [SYS.08][SYS.25] |
| v3.4 Hang On (1988) | 128 KB | yes | $2 bug | `mpr-11458.rom` 8edf7ac6 51fd6d7990f62cd9d18c9ecfc62ed7936169107e | [SYS.08][SYS.25] |
| v4.4 Missile Defense 3-D (1988) | 128 KB | yes | no Snail Maze | `missiled.rom` e79bb689 aa92ae576ca670b00855e278378d89e9f85e0351 | [SYS.08][SYS.25] |
| Alex Kidd (1990, most SMS2) | 128 KB | yes | $2 bug fixed; static logo | `mpr-12808.ic2` cf4a09ea 3af7b66248d34eb26da40c92bf2fa4c73a46a051 | [SYS.08][SYS.25] |
| Sonic (1992, SMS2 EU) | 256 KB | yes | checksum code as v1.3 | `sonbios.rom` 81c3476b 6aca0e3dffe461ba1cb11a86cd4caf5b97e1b8df | [SYS.08][SYS.25] |
| Korean Alex Kidd | 128 KB | yes | non-$FF bytes in first 16 of each slot | `akbioskr.rom` 9c5bad91 2feafd8f1c40fdf1bd5668f8c5c02e5560945b17 | [SYS.08][SYS.25] |
| Game Gear (Majesco/VA1+) | 1 KB | none | TMR SEGA at $7FF0/$3FF0/$1FF0, else screen off and lock-up | `majbios.rom` 0ebea9d4 914aa165e3d879f060be77870d345b60cfeb4ede | [SYS.08][SYS.25] |

Digests are from MAME `sms.cpp` (BSD-3-Clause) at the pin and are used only to recognise a user's own dump; no BIOS byte enters the
repository (AGENTS.md rule 3). MAME's `hash/sms.xml` was not needed and was not checked at pin.
- Boot flow (export, v1.3 onwards): run from RAM; try card, then cartridge, then expansion; look for the header at $7FF0, $3FF0, $1FF0;
  compare "TMR SEGA"; v1.3 wants the region nibble = 4 and a checksum over the range chosen by the size nibble ($A = 8 KB … $2 = 1 MB,
  $3–$9 unsupported), else SOFTWARE ERROR; store the final $3E value at $C000; jump to $0000 [SYS.08][SYS.04][SYS.09].
- Hand-off facts: SMS BIOSes leave $3E = $AB (expansion off, cartridge on, card off, RAM on, BIOS off, I/O on); the GG BIOS leaves $A8
  [SYS.09]. The SMS BIOS also leaves VDP register state and RAM residue that some games rely on [SYS.09]; ares writes $C000 = $AB and
  $C700 = $9B ("VDP $01 initial value") and starts with SP = $FFFD, ares clears RAM, MAME sets GG SP = $DFF0 for Ecco [SYS.28][SYS.26].

**C7 Fast-boot hand-off (no BIOS bytes; all values are facts, none copied).** Recommendation:

| State | SMS (any) | Game Gear | Mark III |
|---|---|---|---|
| Z80 | PC $0000, SP $DFF0, AF/BC/DE/HL/IX/IY $0000 except AF = $FFFF (reset), I = R = 0, IM 1 not set (IM 0), IFF 0 | same | same |
| Port $3E, $C000 | $AB written to the port and to $C000 | $A8 to the port; $C000 untouched (whether the GG BIOS stores it is unverified) | no port |
| Port $3F | $FF (all inputs) | $FF | $FF |
| Mapper | $FFFC $00, $FFFD 0, $FFFE 1, $FFFF 2 (C4) | same | same |
| VDP | registers as SMS-VDP.md's "after BIOS" table (MAME reset: all 0 except R2 = $0E, R10 = $FF [SYS.27]); VRAM, CRAM zero; status flags clear | GG mode unless the cartridge sets SMS mode | same |
| PSG, $F2 | all channels silent; $F2 = %00 (PSG on, FM muted) [SYS.19] | $06 = $FF [SYS.09] | $F2 %00 |
| Beam | tick 0 of line 0, frame 0 | same | same |
| RAM | `ram_fill` (C8), then the two bytes above | `ram_fill` | `ram_fill` |

Strongest alternative: boot from the Z80 reset state alone (SP $FFFF, $3E untouched) as ares does. Would change if: a title boots on
hardware only with the BIOS's VDP residue (the "after BIOS" table) or RAM residue; then the table grows, never the BIOS bytes.

## 7. Reset and pause

- **Pause** is a console button (Game Gear: none) on the Z80 /NMI through the I/O chip; it is an edge, holding it does nothing, and the
  handler at $0066 ends with RETN [SYS.15][SYS.09][SYS.14]. The VDP synchronises it: /NMI is asserted at the start of the last line of the
  frame (NTSC line 261, PAL 312) no matter when the button went down [SYS.11]. MAME drives it through the VDP's NMI input; ares fires on
  the press edge without the line alignment [SYS.25][SYS.30]. On a Game Gear in SMS mode the Start button is the NMI; in GG mode it is only
  $00 bit 7 [SYS.26][SYS.30].
- **Reset button** exists only on the export SMS1. It is not wired to the Z80: it is a software input on $DD bit 4 that games poll and may
  ignore; every other model reads it unpressed [SYS.16][SYS.09]. The SC-3000 keyboard reset goes to NMI instead [SYS.16].
- **What survives.** No model has a CPU reset line reachable by the user; a "reset" is a power cycle. Recommendation: `cogmaster_reset`
  (soft) re-applies C7 and keeps work RAM, cartridge RAM, VRAM and CRAM; `cogmaster_power` (hard) also applies `ram_fill` and zeroes VRAM
  and CRAM. Strongest alternative: a single hard reset. Would change if: a movie/TAS consumer wants a soft reset that matches a
  hardware power-cycle residue; then soft reset becomes hard reset plus a documented residue model.
**C8 Pause alignment.** Recommendation: latch the host's pause press and assert /NMI at tick 611 of the last frame line, as measured
[SYS.11]. Strongest alternative: immediate edge (ares). Would change if: a pause-timing test on hardware disagrees with [SYS.11].

## 8. Power-on profile fields

| Field | Hardware | Default | Pinned by | Sources |
|---|---|---|---|---|
| `ram_fill` (8 KB work RAM) | SRAM pattern, unmeasured; BIOS leaves residue | ZERO; SEEDED(seed) option; profile JAPAN may choose $F0 (MAME's choice) | Shanghai II (GG) hangs on non-zero (ares) | [SYS.28][SYS.26] |
| VRAM, CRAM | undefined | zero | | [SYS.29][SYS.27] |
| VDP registers | undefined; BIOS writes them | SMS-VDP.md's table; MAME: zeros, R2 $0E, R10 $FF | VDP phase vectors | [SYS.27] |
| VDP status, line counter, H latch | | clear, 0, 0 | | [SYS.27][SYS.29] |
| Mapper $FFFC–$FFFF | undefined on most chips | 0, 0, 1, 2 | C4 | [SYS.03] |
| Port $3E | undefined; BIOS leaves $AB/$A8 | $AB SMS, $A8 GG, absent Mark III | C7 | [SYS.09] |
| Port $3F | | $FF | MAME reset | [SYS.26] |
| Port $F2 | %00 on FM machines | %00 | | [SYS.19] |
| GG $00–$06 | | $C0 $7F $FF $00 $FF $00 $FF | | [SYS.09] |
| PSG | undefined | all four attenuations $F, tone 0, noise reset (SMS-AUDIO.md) | | |
| Z80 | PC, I, R, IFF 0, IM 0; AF = SP = $FFFF (unverified) | as C7, SP $DFF0 | | [SYS.23][SYS.24] |
| Open-bus byte | | $FF | C5 | [SYS.28] |
| Beam | undefined phase vs. CPU is fixed by the clock ratio | line 0, tick 0 | C1 | derived |

Every field lives in the instance, the save state and the movie header; BIOS bytes are an input, never a default.

## 9. Open questions

(1) Frame/line /INT position: 607/608 ticks (Charles) against ≈ 654 (FluBBa's H counter reading) [SYS.11]; the VDP timing phase writes
a counter-and-interrupt vector and the owner's hardware, if ever run, settles it. (2) H counter jump point (588 per Sega, unconfirmed)
and the exact read mapping: the VDP phase. (3) SMS1 open-bus byte = last instruction byte: which byte for each instruction form (the
operand, or the refresh?) is untested; the bus phase adds a vector and C5 stays until it fails. (4) Full versus partial port decode on
the Japanese SMS and Mark III ("unconfirmed" in ares, asserted in MAME) [SYS.28][SYS.25]: the machine-profile phase follows MAME.
(5) $F2 on export units: $DD mirror (ares, port map) or fixed %10 (SMS Power!) [SYS.28][SYS.12][SYS.19]: the FM phase; the
Phantasy Star detector fails either way. (6) Whether the GG BIOS stores $3E at $C000, and the VDP "after BIOS" register image: the boot
phase reads the public disassemblies for facts only. (7) Sean Young's reset values and prefix rule (unverified here): the CPU phase
re-fetches the document and SingleStepTests/z80 pins the behaviour.

## Sources

| ID | Tier | Source (URL) | Pin or access date | Supports |
|---|---|---|---|---|
| SYS.01 | T2 | SMS Power! Clock Rate, https://www.smspower.org/Development/ClockRate | accessed 2026-10-18 | crystals, Z80 = crystal/15 |
| SYS.02 | T2 | SMS Power! Memory Map, https://www.smspower.org/Development/MemoryMap | accessed 2026-10-18 | map, mirrors, register addresses |
| SYS.03 | T2 | SMS Power! Mappers, https://www.smspower.org/Development/Mappers | accessed 2026-10-18 | $FFFC bits, first KB, 315-5235 reset, bank widths |
| SYS.04 | T2 | SMS Power! Port $3E, https://www.smspower.org/Development/Port3E | accessed 2026-10-18 | $3E bits, BIOS slot order, GG BIOS window |
| SYS.05 | T2 | SMS Power! Region Detection, https://www.smspower.org/Development/RegionDetection | accessed 2026-10-18 | TH readback test, GG $00 bit 6, region matrix |
| SYS.06 | T2 | SMS Power! TV Type Detection, https://www.smspower.org/Development/TVTypeDetection | accessed 2026-10-18 | 262/313, loop counts |
| SYS.07 | T2 | SMS Power! SMS1/SMS2 VDPs, https://www.smspower.org/Development/SMS1SMS2VDPs | accessed 2026-10-18 | 315-5124 vs 315-5246 |
| SYS.08 | T2 | SMS Power! BIOSes, https://www.smspower.org/Development/BIOSes | accessed 2026-10-18 | BIOS list, checks, quirks, GG BIOS |
| SYS.09 | T2 | Charles MacDonald, SMS/GG hardware notes, https://www.smspower.org/uploads/Development/smstech-20021112.txt | 2002-11-12 | port decode, $3E/$3F/$DC/$DD, open bus, IM0/IM2 bus, $3E hand-off values, GG $00–$06 image |
| SYS.10 | T2 | Charles MacDonald, SMS VDP documentation, https://www.smspower.org/uploads/Development/msvdp-20021112.txt | 2002-11-12 | 342 dots, line layout, V counter tables, interrupt rules, lost CRAM writes |
| SYS.11 | T4, measured | SMS Power! forum "SMS Display Timing" (Charles MacDonald, FluBBa, Christophe), https://www.smspower.org/forums/8161-SMSDisplayTiming | accessed 2026-10-18 | 684-tick line, /INT at 607/608, NMI alignment, V counter read window, H jump 588 |
| SYS.12 | T2 | SMS Power! I/O Port Map (SMS), https://www.smspower.org/Development/IOPortMap | accessed 2026-10-18 | per-model port table, open bus vs $FF |
| SYS.13 | T2 | SMS Power! I/O Port Map (GG), https://www.smspower.org/Development/IOPortMapGG | accessed 2026-10-18 | $00–$06 in GG and SMS modes |
| SYS.14 | T2 | SMS Power! Z80 Interrupt Mechanism, https://www.smspower.org/Development/InterruptMechanism | accessed 2026-10-18 | level /INT, edge /NMI, IM modes, bus byte |
| SYS.15 | T2 | SMS Power! Pause Button, https://www.smspower.org/Development/PauseButton | accessed 2026-10-18 | NMI path, handler convention |
| SYS.16 | T2 | SMS Power! Reset Button, https://www.smspower.org/Development/ResetButton | accessed 2026-10-18 | software-read reset, which models |
| SYS.17 | T2 | SMS Power! Scanline Counter, https://www.smspower.org/Development/ScanlineCounter | accessed 2026-10-18 | V counter sequences |
| SYS.18 | T2 | SMS Power! FM Chip Detection, https://www.smspower.org/Development/FMChipDetection | accessed 2026-10-18 | Phantasy Star detector, $3E bit 2 |
| SYS.19 | T2 | SMS Power! Audio Control Port, https://www.smspower.org/Development/AudioControlPort | accessed 2026-10-18 | $F2 bits, counter bits, Mark III |
| SYS.20 | T2 | SMS Power! RAM, https://www.smspower.org/Development/RAM | accessed 2026-10-18 | D4168, A14/A15 select, $3E bit 4, SP convention |
| SYS.21 | T2 | SMS Power! GG VDP, https://www.smspower.org/Development/GGVDP | accessed 2026-10-18 | GG mode set by cartridge, 160×144 |
| SYS.22 | T2 | SMS Power! Game Gear Stereo, https://www.smspower.org/Development/GameGearStereo | accessed 2026-10-18 | $06 bits |
| SYS.23 | T1 | Zilog Z80 CPU User Manual UM008011-0816, https://www.zilog.com/docs/z80/um0080.pdf | UM008011-0816 | M/T cycles, WAIT, INT/NMI sampling, IFF, reset |
| SYS.24 | T2 | Sean Young, The Undocumented Z80 Documented v0.91, http://www.z80.info/zip/z80-documented.pdf | not fetched (TLS failure) | AF/SP reset, prefix rule (unverified) |
| SYS.25 | T3 | MAME `src/mame/sega/sms.cpp` (BSD-3-Clause), https://github.com/mamedev/mame | `1d7a454ed79e` | clocks, port maps per model, BIOS digests, mirrors note |
| SYS.26 | T3 | MAME `src/mame/sega/sms_m.cpp` (BSD-3-Clause) | `1d7a454ed79e` | $3F/$DD region logic, bus ANDing, GG SMS mode, RAM $F0, SP $DFF0, $F2 read |
| SYS.27 | T3 | MAME `src/devices/video/315_5124.cpp` (BSD-3-Clause) | `1d7a454ed79e` | hpos event table, reset registers, no access constraints |
| SYS.28 | T3 | ares `ares/ms/cpu/cpu.cpp`, `memory.cpp` (ISC), https://github.com/ares-emulator/ares | `9408cb43d494` | mdr open bus, $C1 decode, $3F bits, GG SIO ports, power() writes |
| SYS.29 | T3 | ares `ares/ms/vdp/vdp.cpp`, `io.cpp` (ISC) | `9408cb43d494` | 684 clocks/line, frame flag line, counter queries |
| SYS.30 | T3 | ares `ares/ms/system/system.cpp`, `controls.cpp` (ISC) | `9408cb43d494` | models/regions, pause edge, GG Start NMI in SMS mode |

Licence check at pin: MAME files above carry `license:BSD-3-Clause` headers [SYS.25][SYS.27]; ares `LICENSE` is ISC, copyright 2004–2025
ares team, Near et al. [SYS.28]. Not fetched: SMS Power! Peripheral Ports and Memory and I/O Ports pages (timeouts), Sega Retro (blocked).
