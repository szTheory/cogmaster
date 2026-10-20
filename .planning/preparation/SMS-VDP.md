---
title: "SMS-VDP: revisions, registers, mode 4 pipeline, timing, geometry, palettes, legacy modes"
summary: "What vdp.c must do: the 315-5124/5246/5378 differences games rely on, every register bit, the control port and read-ahead buffer, status flags, the mode 4 line pipeline, sprite rules, H/V counters, line and frame interrupt timing, VRAM access limits, display geometry, SMS and GG palettes, the GG window, TMS9918 legacy modes, test ROMs and the accuracy model."
read_when: "Before writing or changing vdp.c, the frame format, interrupt timing, the palette helpers or the GG profile, or when a game's picture, flicker or raster effect diverges."
updated: 2026-10-18
---

# SMS-VDP

## Where to look

| Question | Section |
|---|---|
| Which VDP revision does what; detection tricks games and tests use | 1 |
| Register bits, control port, address codes, read-ahead buffer, status flags | 2 |
| Name table, tiles, scroll, sprites, overflow, collision, zoom, X-8 shift | 3 |
| Clocks, line timeline, H and V counters, line/frame interrupt timing, mid-line writes, VRAM access limits | 4 |
| Visible area, borders, aspect ratio, frame rates, frame format | 5 |
| SMS and GG CRAM, GG window, GG in SMS mode, output pixel format | 6 |
| TMS9918 legacy modes on the SMS and GG (later seed) | 7 |
| Hard games, test ROMs and their licences, divergences users notice | 8 |
| Accuracy model (tick, per-line with catch-up, when per-dot) | 9 |
| Open questions and the phases that settle them | 10 |

## 1. Revisions
**1.1 Chips** [VDP.01][VDP.04][VDP.05]

| Chip | Machine | Mode 4 heights | SMS1-only quirks | Zoom | Legacy modes |
|---|---|---|---|---|---|
| 315-5124 | Mark III, SMS1 (a few SMS2 shells) | 192 only; M1/M3 with M4 give the invalid text mode | reg 2 bit 0 name-table mask, reg 3/4/5/6 unused bits act as AND masks | H zoom on part of the line only (§3.6) | 0–3 and mixes, fixed palette |
| 315-5246 | SMS2 (a few SMS1 shells) | 192, 224, 240 (240 rolls on NTSC) | none | full | same |
| 315-5378 (MAME names it 315-5377, "2-ASIC") | Game Gear | 192, 224 (240 freezes the LCD until power cycle) | none | full | legacy modes read CRAM 16–31, never initialised |
| 315-5313 | Mega Drive in SMS mode (precedent only) | 192; no 224/240 | no TMS modes (black screen); zoom bit ignored; CRAM is code bit 1; data writes do not load the read buffer; first control byte updates the address low byte at once | none | none |

- Detection in the field: sverx's SMS Test Suite enables zoom, puts two sprites at (0,0) and (15,0) and reads the collision flag. No collision in the vertical test = 315-5313; collision in the horizontal test = 315-5246 or 315-5378; otherwise 315-5124 [VDP.20]. Games detect nothing; they rely on quirks instead (Ys JP on the SMS1 mask, Codemasters titles on 224 lines) [VDP.14][VDP.01].
- SMS Power lists Phantasy Star and Golden Axe Warrior as having reported revision-related problems, undocumented [VDP.04].

**Choice 1.A (revision set).** Recommendation: model 315-5246 as the default SMS, 315-5124 as a profile switch (mask bits, zoom quirk, no 224/240), 315-5378 as the GG profile; never the 315-5313. Strongest alternative: 315-5124 default (what most NTSC/JP users own). Would change if: the conformance corpus shows more titles broken by SMS2 behaviour than by SMS1 behaviour (none known).

## 2. Registers, ports, status
**2.1 Ports.** $7E V counter / $7F H counter (reads; writes go to the PSG), $BE data, $BF control; decoded on A7, A6, A0, so $80–$BF even = data, odd = control [VDP.01].

**2.2 Control port.** Two bytes: first = address bits 7–0; second = bits 13–8 plus code in bits 7–6. The two-byte flag clears on a control-port read or any data-port access. Code 0: load the read buffer from VRAM and increment; 1: VRAM write; 2: register write (reg = second byte bits 3–0, data = first byte; regs 11–15 ignored); 3: CRAM write. Addresses wrap at $3FFF [VDP.01][VDP.06].

**2.3 Data port.** Every access auto-increments. A read returns the buffer, then refills it from VRAM at the new address whatever the code. A write also stores the byte in the buffer (not on the 315-5313) [VDP.01][VDP.05].

**2.4 Registers** (bits that games use are marked) [VDP.01][VDP.02][VDP.06]

| Reg | Bit | Meaning |
|---|---|---|
| 0 | 7 | vertical scroll lock: columns 24–31 use vscroll 0 (status columns) |
| 0 | 6 | horizontal scroll lock: lines 0–15 use hscroll 0 (status bars); invisible in GG mode |
| 0 | 5 | mask column 0 with the backdrop colour (hides scroll garbage); no effect on GG |
| 0 | 4 | IE1 line interrupt enable (gates /INT only; the flag still latches) |
| 0 | 3 | EC: sprites 8 px left (32 px in TMS modes) |
| 0 | 2 | M4 |
| 0 | 1 | M2 (must be 1 for M1/M3 to pick 224/240 on SMS2/GG) |
| 0 | 0 | external sync / monochrome; keep 0 |
| 1 | 7, 2 | no effect on SMS; bit 2 breaks Mega Drive mode 4 |
| 1 | 6 | BLK display enable (0 = backdrop everywhere, sprites not evaluated) |
| 1 | 5 | IE0 frame interrupt enable |
| 1 | 4 | M1 (with M2 + M4: 224 lines on SMS2/GG) |
| 1 | 3 | M3 (with M2 + M4: 240 lines) |
| 1 | 1 | 8x16 sprites (tile index bit 0 ignored) |
| 1 | 0 | zoom: sprites 16x16 / 16x32 |
| 2 | 3–1 | name table address bits 13–11; 224/240 modes use bits 3–2 and base $0700/$1700/$2700/$3700; SMS1: bit 0 ANDs address bit 10 (§3.1) |
| 3, 4 | all | unused in mode 4 on SMS2/GG; SMS1 wants reg 3 = $FF and reg 4 bits 2–0 = 1 or addresses are masked |
| 5 | 6–1 | SAT address bits 13–8; SMS1: bit 0 clear fetches X/tile from the low 128 bytes |
| 6 | 2 | sprite pattern index bit 8 (tiles 256–511); SMS1: bits 1–0 mask address A12/A11, tile index bits 7/6 (VDP.02 register diagram, rechecked 2026-10-20) |
| 7 | 3–0 | backdrop/border colour, from the sprite palette (CRAM 16 + n) |
| 8 | 7–0 | hscroll (bits 7–3 start column, 2–0 fine) |
| 9 | 7–0 | vscroll (latched once per frame, §3.3) |
| 10 | 7–0 | line counter reload value |

**2.5 Mode select** (M4 M3 M2 M1) [VDP.13][VDP.01]: 0xxx = TMS modes (§7). 1000, 1010, 1100 = mode 4 at 192 on every chip. 1011 = 224 and 1110 = 240 on SMS2/GG, the invalid text mode on SMS1. 1111 = mode 4 on SMS2/GG, invalid text mode on SMS1. 1001 and 1101 = invalid text mode everywhere (CRAM colours, each tile repeated four times, 6-px cells).

**2.6 Status byte** ($BF read) [VDP.01][VDP.06][VDP.07]: bit 7 frame interrupt pending, bit 6 sprite overflow, bit 5 sprite collision. Bits 4–0 are garbage on SMS1/SMS2 (MiSTer returns 1s, ares returns the overflow index, MAME an index derived from the overflowing sprite). A read returns the byte, then clears all three flags and the line-interrupt pending flag, and releases /INT.

**Choice 2.A (status low bits).** Recommendation: return 1s (MiSTer; matches "garbage" that never clears a test). Strongest alternative: ares's overflow index. Would change if: a title or VDPTEST checks a value.

## 3. Mode 4 pipeline
**3.1 Name table.** 32x28 words at the reg 2 base (192 lines; the virtual map is 224 px tall, vscroll wraps at 224) or 32x32 words at base $x700 (224/240; wraps at 256). Word: bits 8–0 tile (0–511), 9 hflip, 10 vflip, 11 palette (sprite palette), 12 priority (over sprites where the sprite pixel is opaque, except colour 0 never has priority), 13–15 free (games keep collision flags there). Row stride 64 bytes [VDP.09][VDP.06]. SMS1: when reg 2 bit 0 = 0, address bit 10 is forced 0, so rows 16–27 mirror rows 0–11; only Ys (JP) depends on it; homebrew assuming SMS2 shows it as a bug [VDP.14][VDP.06].

**3.2 Tiles.** 8x8, 4 planar bitplanes, 32 bytes per tile, 4 bytes per row (planes 0–3), 512 tiles share VRAM with the tables [VDP.01]. The VDP fetches each 8-px column as name low, name high, plane 0–3 (MiSTer phases 000–110), then loads the shifter at phase 111 [VDP.07].

**3.3 Scroll.** hscroll: start column = 32 − (reg 8 >> 3), fine = reg 8 & 7; the first fine-scroll pixels of column 0 show backdrop (SMS1 draws some sprite-0 data there, "exact logic unknown"), which is why games set reg 0 bit 5 [VDP.01][VDP.09]. hscroll is sampled once per line, late in the previous line (§4.2), so HBlank writes give per-line effects [VDP.09][VDP.05][VDP.07]. vscroll: Charles and the wiki say reg 9 changes take effect only after the active display, i.e. once per frame, and blanking does not count [VDP.01][VDP.09]; ares and MiSTer add it per line (ares latches it in `setup()` every line; MiSTer sums `y + bg_scroll_y` live) [VDP.06][VDP.07]. See §10.

**3.4 Sprite attribute table.** 256 bytes: $00–$3F Y (64 sprites), $40–$7F unused, $80–$FF X, tile pairs. Shown line = Y + 1. Y = $D0 ends the table in 192-line mode only. Y ≥ $F0 is treated as negative (ares, MiSTer). X does not wrap past 248. EC subtracts 8. 8x16: tile index bit 0 ignored, lower half = index + 1 [VDP.01][VDP.08][VDP.06][VDP.07].

**3.5 Line evaluation.** During line n (MiSTer x = 257–496, i.e. the blanking after the active pixels; MAME parses `line − 1`) the VDP scans the Y table for sprites covering line n + 1 using the current height (8, 16, 32 with zoom) and fills an 8-entry buffer. The scan stops at 64 entries, a full buffer, or $D0. A ninth match sets overflow (bit 6) even if the sprite is off-screen in X or fully transparent; off-screen sprites consume slots on GG too. Lower buffer index wins between sprites [VDP.01][VDP.08][VDP.07][VDP.05]. Overflow is only flagged on lines inside the active area (MAME: and not while VINT is active; MiSTer: y < 234 or ≥ 496) [VDP.05][VDP.07].

**3.6 Zoom.** 8x8 to 16x16, 8x16 to 16x32; the pattern row is `delta >> 1`. SMS1 quirk: not every slot is doubled horizontally. msvdp: "the first four sprites on a line zoom both ways, the rest vertically only" [VDP.01]; the wiki's measured table says the *last four* slots lose horizontal zoom: 2 sprites = VV, 4 = VVVV, 6 = FFVVVV, 8 = FFFFVVVV, so slot i is wide only when a sprite exists in slot i + 4 [VDP.08]. sverx's detection (two zoomed sprites 15 px apart, no collision on an SMS1) agrees with the wiki, not with MAME's "first 4 of 8" model [VDP.20][VDP.05]. Neither ares nor MiSTer models the quirk [VDP.06][VDP.07].

**Choice 3.A (SMS1 zoom).** Recommendation: the wiki model, `wide(i) = (i + 4 < count)`. Strongest alternative: MAME's `i < 4`. Would change if: VDPTEST or a hardware capture shows a lone zoomed sprite drawn 16 px wide on an SMS1.

**3.7 Collision.** Bit 5 sets when opaque pixels of two sprites land on the same dot, pixel level, evaluated while the dot is rendered inside the active area with the display enabled; column 0 under the mask does not count (MAME); the status bit stays until read [VDP.15][VDP.05][VDP.06][VDP.07]. Hardware does not collide off-screen; VDPTEST's "Offscreen Y, col" passes only because of SAT state left by the previous test (SMS Power thread) [VDP.18]. Line-accurate evaluation (flag updated after each line) is enough for all known software [VDP.15]. Games that read it: Cheese Cat'astrophe, Ecco the Dolphin (SMS, GG), Fantastic Dizzy (SMS, GG), Fantasy Zone Gear, Impossible Mission, Taz-Mania (SMS, GG), Master of Darkness [VDP.15].

**3.8 Per-pixel priority.** backdrop → background pixel → sprite pixel (if opaque) → priority tile pixel (if opaque and the tile has bit 12). Left 8 px = backdrop when reg 0 bit 5; display off = backdrop everywhere [VDP.06][VDP.07].

**3.9 Typical VRAM map** (what games set, so a debugger can label it): tiles $0000–$37FF, name table $3800–$3EFF (192 lines) or $3700–$3EFF (224/240), SAT $3F00–$3FFF; reg 6 bit 2 set limits sprites to tiles 256–447 because the tables sit at $3800 up [VDP.09][VDP.02]. The VBlank budget is about 70 lines on NTSC; blanking lines 0–7 and 183–191 through line interrupts adds 16; on the GG, line interrupts at 24 and 168 give 118 usable lines [VDP.25].

## 4. Timing
**4.1 Clocks** [VDP.16][VDP.11][VDP.05][VDP.06]

| Item | NTSC | PAL |
|---|---|---|
| mclk (tick) | 10.738635 MHz (53.693175 / 5; Charles measured 10.738580) | 10.640685 MHz (53.203424 / 5) (derived from the MAME crystal) |
| Pixel clock | mclk / 2 = 5.369318 MHz | mclk / 2 = 5.320342 MHz |
| Z80 | mclk / 3 | mclk / 3 |
| Line | 684 mclk = 342 px (256 active + 15 right border + 8 blank + 26 sync + 2 + 14 burst + 8 + 13 left border) | 684 mclk |
| Frame | 262 lines = 179,208 mclk, 59.9227 Hz | 313 lines = 214,092 mclk, 49.7015 Hz (derived) |

Pixel : Z80 = 3 : 2 in both regions, so one integer tick (mclk) carries CPU, VDP and PSG (mclk/3) exactly.

**4.2 Line timeline** (pixel 0 = first active dot; mclk = 2 × pixel) [VDP.16][VDP.05][VDP.07]

| Event | Charles (logic analyser) | MAME (`line_timing`, 315-5124 / 315-5377) | MiSTer |
|---|---|---|---|
| Active dots | mclk 0–511 | hpos 63–318 of its 342-px screen (LBORDER_START 50 + 13) | x 1–256 |
| hscroll (reg 8) sample for the next line | not measured | hpos ≤ 21 / 24: i.e. the write must land before ≈ pixel 300 of the previous line | x = 487 (cites Nuked-SMS-FPGA: R8 latched at count 488) |
| Display-enable (reg 1 bit 6) sample | not measured | hpos ≤ 24 (Power Strike II needs > 18, Fantastic Dizzy < 25) | per line |
| V counter changes | not measured | hpos 23 / 28 | x = 487; CPU-visible value 2 px earlier (485); "VDPTEST accepts 484/485, rejects 486" |
| Frame /INT | mclk 607 into line 192 (4 mclk before HSYNC rise at 611) | hpos 24 / 26 | x = 485 ("instead of 487 to please VDPTEST") |
| Line /INT | mclk 608 (one Z80 clock after the frame INT; FluBBa) | hpos 26 / 27 | counter evaluated at x = 486 |
| HSYNC | starts mclk 558, 53 wide | pixels 50–75 of the screen | x 280–474 region flag |
| H counter jump | official docs: mclk 588 | none (linear) | x 295 → 466 |

Treat the MAME hpos values as "next line, before active"; they agree with Charles to within a few mclk. In cogmaster terms: frame and line /INT at mclk ≈ 607 of the line, hscroll and display-enable latched at mclk ≈ 600, V counter increments at ≈ 604.

**4.3 H counter.** 9-bit pixel counter, read as the upper 8 bits: $00–$93 for pixels 0–295, then $E9–$FF for pixels 296–341 (171 values) [VDP.01][VDP.23][VDP.06][VDP.07]. ares: `(h ≥ 592 ? h + 340 : h) >> 2` in mclk; MAME has no jump (`((hclock − 46) >> 1) & $FF`) [VDP.06][VDP.05]. The read value is latched when TH changes (Light Phaser); otherwise live [VDP.01]. FluBBa's IRQ tester saw HC = $06/$07 at the first instruction of the interrupt handler (≈ 20 mclk), i.e. /INT lands at the very end of the line [VDP.16].

**4.4 V counter** (reads at $7E) [VDP.01][VDP.03][VDP.05][VDP.07]

| Mode | Lines | Sequence | Active | Bottom border | Blank + sync + blank | Top border |
|---|---|---|---|---|---|---|
| NTSC 192 | 262 | $00–$DA, $D5–$FF | $00–$BF | 24 ($C0–$D7) | 3 + 3 + 13 | 27 ($E5–$FF) |
| NTSC 224 | 262 | $00–$EA, $E5–$FF | $00–$DF | 8 | 3 + 3 + 13 | 11 |
| NTSC 240 | 263 (rolls) | $00–$FF, $00–$06 | $00–$EF | 0 | not functional | 3 |
| PAL 192 | 313 | $00–$F2, $BA–$FF | $00–$BF | 48 | 3 + 3 + 13 | 54 |
| PAL 224 | 313 | $00–$FF, $00–$02, $CA–$FF | $00–$DF | 32 | 3 + 3 + 13 | 38 |
| PAL 240 | 313 | $00–$FF, $00–$0A, $D2–$FF | $00–$EF | 24 | 3 + 3 + 13 | 30 |

ares implements these as "subtract 6 above 218/234" (NTSC) and "subtract 57 above 242/258, 56 above 266" (PAL) [VDP.06]. Software can read the counter twice to dodge the mid-line change [VDP.03].

**4.5 Line interrupt counter.** On every line of the active area and the first line after it (lines 0–192 in 192-line mode) the counter is tested and decremented: at 0 it reloads from reg 10 and sets the pending flag; on every other line (193–261) it is reloaded without a test. Reg 10 writes do not touch the counter until the next reload. The flag persists until the control port is read; IE1 gates /INT only [VDP.01][VDP.06][VDP.07]. MAME places the test at hpos 14 (DISPLAY_CB_HPOS, "fixes Road Rash title scrolling, due to line counter reload timing") and HINT at 26 [VDP.05]. Consequences: reg 10 = n gives an interrupt on line n, then every n + 1 lines; reg 10 = $FF (MiSTer's reset value) never fires inside a frame [VDP.07].

**4.6 Frame interrupt.** Pending flag set at the end of the last active line (line 191 for 192; mclk ≈ 607), which is when the V counter has just become $C1 / $E1 / $F1; ares models it as "vcounter == vlines + 1" at line start [VDP.01][VDP.06][VDP.16]. IE0 gates /INT; the flag stays until the control port is read; /INT follows the flags combinationally (MiSTer adds a 7-clock delay) [VDP.07]. NMI (Pause) is a SYS concern: asserted at the /HSYNC rise of line 260 (PAL 311), the start of the last line [VDP.16].

**4.7 Mid-line writes.** reg 7 (backdrop) and CRAM writes show from the next dot on real hardware (raster bars); reg 8 shows from the next line; reg 9 from the next frame (§3.3); reg 1 display enable from the next line (MAME) [VDP.05][VDP.07]. An accuracy model therefore needs dot-exact *latching positions* but only line-granular *rendering*, provided it renders up to the current dot before applying a CRAM, reg 7, reg 0 or VRAM write (§9).

**4.8 CPU access limits.** The hardware has no wait states; too-fast accesses are lost or mis-addressed. Facts: official Mark III figures 16 T-states between data-port writes in VBlank, 29 during active video [VDP.17]; Maxim (2003, SMS2): `out (c),r` loops corrupted at 12–13 extra cycles, clean at 14 and above, so ≈ 26 cycles minimum during active display [VDP.17]; one cycle-accurate-VDP author found games writing faster, timed to the access windows [VDP.17]; Charles: rapid CRAM writes during active display land at the wrong address or are dropped on SMS2 but not on GG or MD [VDP.01]; sverx's suite detects the older twin-ASIC GG by writing VRAM with a 26-cycle `outi` loop while the V counter is 64–159 and reading it back: mismatches mean twin-ASIC [VDP.20]. The TMS9918A datasheet limits (8 µs worst case in active graphics, 2 µs in blanking) are secondhand here [VDP.24] (unverified).

**4.9 Recommended named constants** (mclk unless stated; all from the tables above)
- `VDP_LINE_TICKS 684`, `VDP_PIXEL_TICKS 2`, `VDP_CPU_TICKS 3` [VDP.16][VDP.06]
- `VDP_LINES_NTSC 262`, `VDP_LINES_PAL 313` [VDP.01]
- `VDP_ACTIVE_END 512` (first blanking tick), `VDP_HSYNC_START 558`, `VDP_HSYNC_END 611` [VDP.16]
- `VDP_INT_TICK 607` (frame and line /INT; line /INT at 608 is one Z80 clock later, keep one constant and order the line test after the frame test) [VDP.16]
- `VDP_LATCH_TICK 600` (reg 8, display enable, next-line sprite scan complete) (derived from MAME hpos 21–24 and MiSTer 487) [VDP.05][VDP.07]
- `VDP_VCOUNT_TICK 604` (CPU-visible V counter increments) (derived from MiSTer 485 vs 487 and MAME 23) [VDP.07][VDP.05]
- `VDP_HCOUNT_JUMP_PIXEL 296` ($93 → $E9) [VDP.06][VDP.07]
- `VDP_SPRITE_SLOTS 8`, `VDP_TMS_SPRITE_SLOTS 4`, `VDP_SAT_TERMINATOR 0xD0`, `VDP_SPRITE_Y_NEGATIVE 0xF0` [VDP.01][VDP.06]
- `VDP_MIN_ACCESS_CYCLES 26` (statistics only) [VDP.17]
- `GG_WINDOW_X 48`, `GG_WINDOW_Y 24` (40 at 224 lines), `GG_WINDOW_W 160`, `GG_WINDOW_H 144` [VDP.05][VDP.07]

**Choice 4.A (access limits).** Recommendation: v1 never corrupts; count data-port accesses closer than 26 Z80 cycles during active lines in the per-frame statistics so the conformance phase sees which titles depend on it. Strongest alternative: a per-line slot model (MD-style counts are unknown for the SMS). Would change if: a corpus title shows garbage that hardware also shows (none known), or a hardware slot schedule is published.

## 5. Display geometry
**5.1 Visible area.** 256 × 192/224/240 active; borders 13 left, 15 right, 27/11 top, 24/8 bottom (NTSC); PAL borders 54/48 (192) and 38/32 (224) [VDP.01][VDP.05]. Borders and the masked column show CRAM[16 + reg 7]; blanking lines are black [VDP.01].

**5.2 Aspect.** SMS NTSC pixel aspect 8:7 (5.369 MHz dot, 4:3 picture = 280 dots); PAL 2950000:2128137 ≈ 1.386 (18:13 within 0.11 %); GG LCD 6:5 (160x144 on a 4:3 panel); GG in SMS mode ≈ 10:9 [VDP.11]. ares presents CRT 8:7 (NTSC) and 19:14 (PAL) [VDP.06].

**5.3 Frame rates.** 59.9227 Hz and 49.7015 Hz (§4.1): not 60/50, so hosts resample audio or use VRR.

**Choice 5.A (frame format).** Recommendation: the core renders the full 256 × H field into an internal buffer every frame and exposes a crop rectangle: SMS (0, 0, 256, H); GG (48, 24, 160, 144), 224-line GG (48, 40, 160, 144) [VDP.05][VDP.07][VDP.06]. The frame hash covers the crop, with the backdrop colour and the border geometry as frame metadata so a host can draw borders. Reported PAR 8:7 / 1.386 / 6:5. CRT vs LCD (scanlines, GG ghosting, GG colour bleed) are host helpers outside the hash. Strongest alternative: hash the whole 342 × 262 raster. Would change if: a test ROM comparison needs border pixels.

## 6. Palettes
**6.1 SMS CRAM.** 32 bytes, `--BBGGRR`, entries 0–15 background palette, 16–31 sprite palette (also usable by tiles with bit 11); backdrop = entry 16 + reg 7; addresses mirror past 32; writes take effect immediately [VDP.01][VDP.10]. SMS1/Mark III output level for blue step 1 is brighter: MAME levels SMS1 {0, 78, 160, 238} with blue {0, 98, 160, 238}; SMS2 {0, 89, 174, 255}; 315-5313 in SMS mode {0, 99, 162, 255} [VDP.05][VDP.10].

**6.2 GG CRAM.** 32 entries × 12 bits `----BBBBGGGGRRRR`, written as two bytes: the even-address byte goes to a latch, the odd-address byte commits latch + low nibble as one word (write $FF to $0000 then $0F to $0021 sets entry 16 to $0FFF and leaves entry 0 alone) [VDP.01][VDP.06][VDP.07]. In SMS mode (SMS cartridge in a GG, or a GG game that never enters GG mode) the CRAM behaves as on the SMS, and each 2-bit channel is shown duplicated into 4 bits (`c | c << 2`) [VDP.01][VDP.06][VDP.07].

**6.3 GG window.** GG mode shows columns 48–207 and rows 24–167 of the 256 × 192 field (rows 40–183 in 224-line mode); 240-line mode freezes the LCD [VDP.05][VDP.07][VDP.06][VDP.12]. The scroll-lock bits still act: rows 0–15 are off-screen, columns 24–31 partly visible [VDP.12]. In SMS mode the LCD controller crops 8 columns each side (240 wide) and maps 3 dots to 2 LCD pixels with sub-pixel colour mixing (fringing); vertically a 192-line image is padded by 13 border rows top and bottom and scaled 218 → 146, a 224-line image has 3 rows trimmed each side and fills the panel [VDP.12][VDP.01].

**6.4 TMS palette on the SMS.** Fixed 16 entries mapped to 6-bit values: 0/1 $00, 2 $08, 3 $0C, 4 $10, 5 $30, 6 $01, 7 $3C, 8 $02, 9 $03, A $05, B $0F, C $04, D $33, E $15, F $3F (darker than a real TMS9918) [VDP.10]. GG legacy modes read CRAM 16–31 instead and show black unless set [VDP.01][VDP.10].

**Choice 6.A (output format).** Recommendation: the core emits one `uint16_t` RGB444 (`0BBBBGGGGRRRR`) per dot, the GG native format, into which SMS 6-bit colours are expanded exactly as the GG does in SMS mode (lossless, reversible); this is the hash domain. A host helper outside the hash converts RGB444 to RGB888 through a selectable level table (SMS1, SMS2, MD-in-SMS-mode, GG linear), seeded from MAME's BSD-3 tables with notice. Strongest alternative: emit CRAM indices plus a per-frame CRAM snapshot (fails on mid-frame CRAM writes). Would change if: a host needs per-dot palette indices for rewind or ROM-hacking tools; then add a debug sideband.

## 7. TMS9918 legacy modes (later seed; facts recorded now)
- Modes on the SMS: 0 Graphics I (32x24 cells, pattern table + 32-entry colour table), 1 Text (40x24 of 6x8, two colours, no sprites, no border select), 2 Graphics II (three 2 KB thirds with per-row colours), 3 Multicolor (64x48 of 4x4 blocks), plus the four undocumented mixes [VDP.13][VDP.22]. All use the fixed palette of §6.4 on the SMS, CRAM 16–31 on the GG, nothing on the MD [VDP.01][VDP.10].
- Sprites: 32 entries × 4 bytes (Y, X, pattern, colour with EC bit 7 = 32 px left), 8x8 or 16x16, zoom, 4 per line; the fifth sets the overflow flag (TMS "5S") and its index in bits 4–0 on a real TMS; $D0 terminates unconditionally [VDP.21][VDP.06][VDP.07]. ares/MiSTer treat Y ≥ $E0 as negative in these modes.
- Graphics II address masking (reg 3 bits 6–0 and reg 4 bits 1–0 AND the character code) exists on a real TMS and on the SMS1; SMS2 and GG do not mask [VDP.21][VDP.01].
- Reg 0 bit 3 (EC) shifts TMS sprites too [VDP.01]. Mode 3 is used by no Sega title [VDP.13].
- Titles: F-16 Fighting Falcon (SMS) runs in a TMS mode and shows black on the MD (msvdp lists it as "F-15") [VDP.01]; SG-1000 cards and the Mark III's SG-1000 compatibility are the main consumers.

## 8. Hard cases and test ROMs
**8.1 Titles and what breaks**

| Title | Needs | Source |
|---|---|---|
| Ys (JP) | SMS1 reg 2 bit 0 mask | [VDP.14][VDP.01] |
| Micro Machines, Cosmic Spacehead (Codemasters) | 224-line mode; Cosmic Spacehead also writes the address low byte with the first control byte | [VDP.01][VDP.13] |
| Fantastic Dizzy | display-enable latch before hpos 25; no collision in the top border | [VDP.05] |
| Power Strike II | display-enable latch after hpos 18 | [VDP.05] |
| Road Rash | line counter test position (title scrolling) | [VDP.05] |
| F-16 Fighting Falcon | TMS mode | [VDP.01] |
| Bart vs. the Space Mutants, Ax Battler (GG) | palette / sprite details that differ on the 315-5313 | [VDP.01] |
| Collision users (§3.7) | pixel-level collision per line | [VDP.15] |
| Every scroller | per-line hscroll latch, column-0 mask, 8-sprite flicker | [VDP.09] |

**8.2 Test ROMs**

| ROM | Author, date | Covers | Licence / availability |
|---|---|---|---|
| SMS VDP Test (VDPTEST.sms) v1.31 | FluBBa, 2009-11-04 | VDP flags, sprite collision incl. off-screen cases, overflow, IRQ and H/V counter timing (MiSTer tuned x = 484/485 to pass it; MAME sms1pal passes all SMS tests) | SMS Power Homebrew page, source included, no licence stated: run as a released binary, do not vendor [VDP.19][VDP.07][VDP.05] |
| IRQ/HCount tester | FluBBa, 2008 | H counter at interrupt entry | forum attachment, no licence [VDP.16] |
| SMS Test Suite | sverx, GitHub, last push 2026-08-31 | PLUGE, colour bars, bleed, grid, stripes, linearity, drop shadow, striped sprite, PSG/FM, pads, VDP type, 50/60 Hz, GG and twin-ASIC detection, BIOS checksum | no LICENSE file at the pin (checked); facts only, run as a binary [VDP.20] |
| Charles MacDonald's test programs | cgfm2.emuviews.com (offline), compilation re-posted on SpritesMind | VDP window and timing tests | licence unknown [VDP.16] |
| Sprite Masking and Overflow Test ROM | Nemesis | Mega Drive only | precedent, not applicable |
| 240p Test Suite | Artemio | Mega Drive; sverx's suite is the SMS analogue | not applicable |

**8.3 Divergences users notice.** Missing 8-sprite flicker or missing sprites (overflow rules), wrong left column (mask and fine-scroll garbage), status bars that scroll (lock bits, reg 8 latch position), raster effects off by a line (interrupt mclk position, line counter reload line), SMS1 zoom (games with sacrificial sprites), 224-line Codemasters titles cut off or rolling, GG colour fringing and the window position, PAL frame rate and aspect.

## 9. Accuracy model
**Choice 9.A.** Recommendation: integer master tick = mclk (684 per line; Z80 = 3 ticks; pixel = 2). The VDP keeps `line` and `hpos` and renders per line into the field buffer with catch-up: before applying any write to CRAM, reg 0, 1, 7, VRAM, or the SAT during an active line, render dots up to the current hpos; at mclk 600 latch reg 8 and the display-enable bit for the next line and run the sprite scan for the next line; at mclk 607 raise the frame or line interrupt per §4.5–4.6; V counter reads use the §4.4 tables with the +1 after mclk 604. Collision and overflow are computed in the line renderer (line-accurate, §3.7). Strongest alternative: a per-dot state machine (MiSTer-style fetch phases). Would change if: VDPTEST's timing items or a title needs sub-line fetch effects (mid-line VRAM writes landing in a tile already fetched, SMS1 column-0 sprite garbage), or if the conformance phase finds a raster effect wrong by less than a line. The measurements that decide: VDPTEST pass list on cogmaster vs MAME/ares, Charles's 607/608 figures reproduced by a counter-reading harness ROM, Road Rash title scroll.

## 10. Open questions (settled by phase)
- **vscroll per frame or per line** (§3.3): documents say per frame, ares and MiSTer apply per line. Phase: VDP timing; decide with VDPTEST and a harness ROM that writes reg 9 mid-frame; default to per frame.
- **SMS1 zoom slot rule** (§3.6): wiki/sverx "last four" vs MAME "first four". Phase: SMS1 profile; default wiki.
- **Interrupt and latch positions** (§4.2): Charles 607/608 mclk vs MiSTer x = 485–488 vs MAME hpos 21–28; differences of a few mclk matter only for H-counter-polling code. Phase: VDP timing; default Charles.
- **Overflow and collision outside the active area** (§3.5, 3.7): MAME and MiSTer gate differently. Phase: sprites; default "active lines only, display on".
- **Access-slot model** (§4.8): no SMS schedule is published. Phase: later seed.
- **TMS modes on SMS vs GG** (§7): seed; needs the TMS9918A datasheet text (fetched only as a scanned PDF here).
- SMS Power pages and forums returned 503 during this pass; forum facts [VDP.17][VDP.18][VDP.19] come from search excerpts and should be re-read at the pin before a phase relies on an exact number.

## Sources

| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| VDP.01 | T2 | Charles MacDonald, "Sega Master System VDP documentation", msvdp-20021112.txt (smspower.org/uploads/Development/) | accessed 2026-10-18 | ports, control port, buffer, registers, modes, name/sprite tables, counters, interrupts, CRAM, GG, 315-5313, CRAM write loss |
| VDP.02 | T2 | smspower.org/Development/VDPRegisters | accessed 2026-10-18 | register bits, SMS1 mask bits |
| VDP.03 | T2 | smspower.org/Development/ScanlineCounter | accessed 2026-10-18 | V counter sequences |
| VDP.04 | T2 | smspower.org/Development/SMS1SMS2VDPs | accessed 2026-10-18 | revision pairing, undocumented game reports |
| VDP.05 | T3 | github.com/mamedev/mame `src/devices/video/315_5124.cpp`, `.h` (BSD-3, header checked) | 1d7a454ed79e | line_timing hpos, border tables, zoom 4/8, overflow/collision gating, GG window, levels, Road Rash, Dizzy, Power Strike II |
| VDP.06 | T3 | github.com/ares-emulator/ares `ares/ms/vdp/{vdp,io,background,sprite,dac}.cpp` (ISC, LICENSE checked) | 9408cb43d494 | 684 clocks, counters, H jump formula, V tables, per-line scroll latch, SMS1 mask, sprite rules, GG CRAM, viewports |
| VDP.07 | T3, facts only | github.com/MiSTer-devel/SMS_MiSTer `rtl/vdp*.vhd`, `rtl/video.vhd` (no LICENSE file; T80 BSD) | 8443a0d19a88 | x/y counters, 485–488 positions, VDPTEST notes, fetch phases, sprite scan window, overflow/collision gates, GG latch and window |
| VDP.08 | T2 | smspower.org/Development/Sprites | accessed 2026-10-18 | SAT, terminator, overflow, SMS1 zoom table |
| VDP.09 | T2 | smspower.org/Development/Tilemap | accessed 2026-10-18 | map sizes, word format, scroll semantics |
| VDP.10 | T2 | smspower.org/Development/Palette | accessed 2026-10-18 | CRAM formats, TMS mapping, SMS1 blue |
| VDP.11 | T2 | smspower.org/Development/PixelAspectRatio | accessed 2026-10-18 | PAR, pixel clocks |
| VDP.12 | T2 | smspower.org/Development/GGVDP and /GGLCD | accessed 2026-10-18 | GG window, LCD scaling in SMS mode, 240-line freeze |
| VDP.13 | T2 | smspower.org/Development/Modes | accessed 2026-10-18 | M1–M4 table, legacy modes, Codemasters 224 |
| VDP.14 | T2 | smspower.org/Development/TilemapMirroring | accessed 2026-10-18 | SMS1 mask, Ys |
| VDP.15 | T2 | smspower.org/Development/CollisionDetection | accessed 2026-10-18 | collision semantics, games |
| VDP.16 | T4, measured | smspower.org/forums/8161-SMSDisplayTiming (Charles MacDonald 2006, FluBBa 2008) | accessed 2026-10-18 | mclk, 684, 607/608, HSYNC, NMI, HC at IRQ |
| VDP.17 | T4, measured | smspower.org/forums/2126 and /16298 (Maxim 2003; official 16/29 figures) | search excerpts, 2026-10-18; pages 503 | 26-cycle minimum, access windows |
| VDP.18 | T4 | smspower.org/forums/19415-OffScreenSpriteCollisionDetectionBehaviour | search excerpt, 2026-10-18 | no off-screen collision, VDPTEST artefact |
| VDP.19 | T2 | smspower.org/Homebrew/SMSVDPTest-SMS; mametesters.org/view.php?id=6053 | search excerpt / accessed 2026-10-18 | VDPTEST v1.31, FluBBa, 2009, MAME pass |
| VDP.20 | T3, facts only | github.com/sverx/SMSTestSuite `main.c`, README (no LICENSE) | pushed 2026-08-31 | zoom-collision detection, twin-ASIC 26-cycle test, test list |
| VDP.21 | T3 | github.com/mamedev/mame `src/devices/video/tms9928a.cpp` (BSD-3) | master, accessed 2026-10-18 | TMS status bits, mode decode, masking, sprite rules, palette |
| VDP.22 | T2 | en.wikipedia.org/wiki/TMS9918 | accessed 2026-10-18 | mode layouts, variants |
| VDP.23 | T4 | gendev.spritesmind.net/forum/viewtopic.php?p=37865 | accessed 2026-10-18 | H counter $00–$E9 jump to $93 (SMS-derived) |
| VDP.24 | T4 | msx.org forum threads on TMS VRAM access timing (8 µs / 2 µs, 28 vs 14 T-state loops) | accessed 2026-10-18 | TMS access limits (secondhand of the datasheet) |
| VDP.25 | T2 | smspower.org/Development/VDPProgrammingTechniques | accessed 2026-10-18 | VBlank budget, GG line-interrupt windows |
| VDP.26 | T1 | TI TMS9918A/9928A/9929A datasheet (cs.columbia.edu/~sedwards/papers/TMS9918.pdf) | fetched as a scanned PDF, text not extracted | marked (unverified) where cited |
