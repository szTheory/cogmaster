# Architecture Patterns: v2.0 "The whole library"

**Domain:** deterministic, integer-only C17 Sega Master System / Game Gear emulator core (library, runner, libretro adapter), adding boards, saves, peripherals and a BIOS path to a shipped v1.0
**Researched:** 2026-11-09
**Overall confidence:** HIGH for integration points (read from `src/`, `include/`, `runner/`, `libretro/` at `b9c6eb7`); MEDIUM for four hardware semantics that are named in "Open decisions" and must be settled inside their phase

Citations: [PREP-ARCH] `.planning/preparation/ARCHITECTURE.md`, [PREP-HOST] `HOSTS-AND-INTEGRATION.md`, [PREP-CART] `SMS-CARTRIDGES-AND-PERIPHERALS.md`, [PREP-SYS] `SMS-SYSTEM.md`, [VIDEO] `docs/hardware/video.md`. File and function names are the shipped ones. No GPL, LGPL, non-commercial or unlicensed source was opened; hardware facts come from the preparation files, which already cite SMS Power, MAME (BSD-3, CC0 lists) and ares (ISC).

## 1. What the code says that the preparation files do not (read this first)

The preparation architecture was written before v1.0 and is out of date in six places that change v2.0 scope. The roadmapper should plan from this list.

| # | Shipped fact | Consequence for v2.0 |
|---|---|---|
| 1 | The public header is API 9 and is much smaller than [PREP-ARCH] section 4: no `save_state`, `load_state`, `get_memory`, `poke`, `get_z80`, hooks or `run`. The state visitor (`src/state.c`, format 11) is internal; libretro `retro_serialize_size` returns 0 and `.info` says `savestate = "false"`. | State-format bumps are free until v3.0 freezes the format. No external consumer breaks. Do not add public state calls in v2.0. |
| 2 | Runner `--save-dir` with atomic replacement is **already shipped** (`runner/save.c`: exclusive temp file, flush, fsync, `rename` or `MoveFileExW`, generation-based dirty check, SIGTERM flush, exit 4 on oversize). | The milestone line "runner `--save-dir` atomic writes" is mostly done. The v2.0 delta is the EEPROM span flowing through the same path, `--save-interval` / SIGUSR1, `.srm` interop evidence, `libretro_saves = "true"` with its guard test, and the hosted save round trip. Re-scope the requirement. |
| 3 | `cogmaster_set_input` does `m->pending_input = *in;` after checking only `size >= 32`. It copies `sizeof` the **current** struct. | The moment `cogmaster_input` grows, a v9 caller with a 32-byte struct is read past its end (ASAN failure, UB). The copy must become `min(tag, sizeof)` with the rest zeroed, in the same change that appends the first axis field. |
| 4 | `io.c` has no device abstraction. `port_dc`/`port_dd` read `m->input.p1/p2` directly; `COGMASTER_DEVICE_*` has only PAD (0) and NONE (1); `th_pins()` derives TH purely from `$3F` and region. | Peripherals need a pin-level device layer first (section 4). It is a refactor of working, hash-pinned code, so it must land with the existing `io.*` tests unchanged. |
| 5 | The H-counter latch exists: `io_control()` calls `cogmaster__vdp_latch_h()` on a low-to-high effective TH edge, and `vdp_hcount()` reads the **current** `bus.t`. | A light gun creates TH edges at ticks earlier than the CPU access that observes them, so the VDP needs an `_at(tick)` form of both the counter and the latch (section 5). |
| 6 | `boot.c` is 31 lines and does the fast-boot hand-off only; `vdp_reset()` always applies "registers as the BIOS leaves them" (R2 $0E, R10 $FF); `io_reset()` doubles as the hand-off for `$3E`; `cogmaster_media` has no BIOS fields; libretro has no firmware lookup, no file I/O, six options, two port devices. | The BIOS path touches boot, bus decode, VDP reset, media, state header, runner flags and the adapter. It is not a one-file feature. |

Also shipped and relevant: `tools/cartdb.c` maps a MAME `slot` string to a board number (`codemasters`=3, `rom`=1, `sega`=2, anything else 0); 255 of 1,657 rows in `src/cartdb.inc` have board 0. `cart.c::other_board()` only decides "unsupported", it never names a Korean board.

## 2. Recommended architecture (v2.0 delta on the v1.0 shape)

The v1.0 invariants hold and each new piece is built to fit them: ticks are VDP clocks; the Z80 drives `bus.c`; devices catch up lazily to the access tick; the VDP renders monotonically and never un-renders; one instance holds all state; no function-pointer tables (`abi.globals`); every call is bounded.

```
 HOSTS      cogmaster-run                          libretro adapter
 (outside   --boot --bios [MOD]  --port1/2 DEV [MOD]   firmware lookup, option cogmaster_boot [MOD]
 the hash)  --save-interval [MOD] input axes [MOD]     controller subclasses, lightgun/analog map [MOD]
                                                       crosshair, 3-D eye compose [NEW, host/]
               |                                       |
 ----------- public API  include/cogmaster.h  (API 9 -> 12, ABI stays 1, append only) ------------
   media: bios, bios_size, boot [MOD]   input: axes [MOD]   run_result.glasses [MOD]   info.boot [MOD]
   board constants [MOD]   devices LIGHT_PHASER/PADDLE/SPORTS_PAD [MOD]   status BIOS_REJECTED [MOD]
               |
 instance.c (load: BIOS copy, power-on vs hand-off) [MOD]    frame.c (apply input, device resets) [MOD]
               |
 bus.c  deadline dispatcher + one `slow` flag [MOD]  ---->  vdp.c  events + hcount_at/latch_at [MOD]
   |  \_ $FFF8-$FFFB snoop (glasses) [MOD]                    vdp_render.c  (unchanged; read-only use by gun)
   |
   +-- mapper.c  Sega, Codemasters, ROM-only (arms untouched) [MOD: new arms only]
   |      +-- mapper_ext.c  Korean family, Zemina/Nemesis, Hicom, 4PAK, Janggun, ... [NEW]
   |      +-- eeprom.c      93C46 bit-bang, image lives in cart.ram [NEW]
   +-- io.c  $3E/$3F/$DC/$DD, TH edge detector [MOD]
   |      +-- ports.c   pin-level devices: pad, paddle, sports pad, (MD pad) [NEW]
   |      +-- lphaser.c Light Phaser: lazy sensor sampler, reads field[][] [NEW]
   +-- boot.c  fast hand-off + power-on + BIOS identify [MOD]      bios window in bus.c [MOD]
   psg.c, opll.c, synth.c  (untouched)
               |
 state.c  visitor: new fields per module, header reserved bytes, tripwires [MOD]
 cartdb.inc / tools/cartdb.c  board + EEPROM geometry columns [MOD]      cart.c  guess_board() [MOD]
```

### Component boundaries

| Component | Status | Responsibility | Communicates with |
|---|---|---|---|
| `src/mapper_ext.c` | NEW | read/write arms for every non-Sega board, one `switch (board)` entry per board, no tables of pointers | `mapper.c` (delegation), `battery.c` (generation), `bus.c` (fault) |
| `src/eeprom.c` | NEW | 93C46 Microwire state machine (CS, CLK, DI, DO, opcode, address, shift, write-enable latch); the 128-byte array is `cart.ram` | `mapper.c` slot-2 window, `battery.c` |
| `src/ports.c` | NEW | pin levels per port per device at a tick: pad, paddle, sports pad, optional MD pad; effective TH | `io.c`, `lphaser.c`, `vdp.c` (latch) |
| `src/lphaser.c` | NEW | sensor sampling grid, lit state, sticky seen flag, TH edge generation | `ports.c`, `vdp.c` (`render_until`, `hcount_at`), `bus.c` catch-up call sites |
| `host/glasses.c` (or in adapter) | NEW | eye select and anaglyph compose from per-frame eye flag; outside every hash | libretro adapter |
| `io.c` | MOD | keeps `$3E/$3F` and port decode; delegates pins to `ports.c`; owns the single TH edge detector and the `$FFF8` snoop | `ports.c`, `vdp.c` |
| `bus.c` | MOD | one precomputed `slow` flag (cart off, RAM off, BIOS on) replaces the per-read `$3E` mask test; BIOS window; `$FFF8-$FFFB` hook; gun catch-up call sites | everything |
| `boot.c` | MOD | `boot_handoff()` unchanged for FAST; new `boot_power_on()` for BIOS; BIOS identification by CRC-32 against a facts table | `instance.c`, `vdp.c`, `io.c` |
| `vdp.c` | MOD | `vdp_hcount_at(t)`, `vdp_latch_h_at(t)`, power-on reset variant; gun catch-up at the line-512 event | `io.c`, `lphaser.c` |
| `cart.c`, `tools/cartdb.c`, `cartdb.inc` | MOD | `guess_board()` names Korean boards from the opcode scan; generator maps new `slot` strings and EEPROM geometry | `mapper_ext.c` |
| `state.c` / `state.h` | MOD | new fields with bounds and cross-checks; header gets BIOS CRC and boot flag from the 11 reserved bytes | every module |
| `runner/*`, `libretro/*` | MOD | flags, option tables, device subclasses, firmware, interval flush | public API only |

## 3. Where each feature attaches

Rows are features, columns are the four attachment surfaces the milestone asked about.

| Feature | Bus / dispatch | State visitor | Public API | Runner and libretro |
|---|---|---|---|---|
| **Korean family** (zemina/MSX, nemesis, korean $A000, korean_nb, hicom, 4pak, janggun, 188-in-1, seojin, 2 Hap) | Cartridge-area writes already reach `mapper_write` (`addr < $C000`), so MSX `$0000-$0003`, 4PAK `$3FFE/$7FFF/$BFFF`, 188-in-1 `$2000`-masked, Korean `$A000` need no bus change. Hicom and Janggun use `$FFFF/$FFFE` (already routed). **Only Seo Jin's `$FFF0`** is outside the current `addr >= $FFFC` gate. | `struct cogmaster__mapper` grows (extra 8 KiB bank regs, janggun flag); tripwire `== 4u` changes; `mapper_io` bounds become per-board (8 KiB banks: `< pages*2`). | Append `COGMASTER_BOARD_*` constants (API +1). `UNSUPPORTED_BOARD` shrinks to the few left out. | `--probe` prints the new board names; ledger rows move from `unsupported` to evaluated. No adapter change. |
| **93C46 EEPROM** | `mapper.c` Sega arm: `$FFFC` b3 window at `$8000-$BFFF` already exists for RAM; the EEPROM board routes slot-2 reads and writes to `eeprom.c` instead of `cart.ram` indexing. b7 resets the chip. | `struct cogmaster__eeprom` (about 8 small fields) visited after the mapper; the 128 B array is `cart.ram`, already visited by `mapper_io` (`ram_size` 128). | none required. `cogmaster_get_battery` already returns a 128 B span with generation. Optionally a `COGMASTER_BOARD_SEGA_EEPROM` constant. | Runner and libretro need **no code**: `RETRO_MEMORY_SAVE_RAM` and `--save-dir` consume the classified span. Work is generator geometry (`declared=128`, `SUPPORTED`), tests, and the interop byte order. |
| **Sports Pad, Paddle** (and MD pad) | `io.c::chip_in` asks `ports.c` for the pin byte; `$3F` writes pass the tick to the device so the sports pad can time its nibble phase from TH edges. Paddle's nibble clock is a pure function of `ticks` (no state), like `$F2`'s line counter. | `cogmaster__io` gains per-port device state: sports pad phase, last edge tick, pending dx/dy; bounded; edge tick `<= ticks`. | `COGMASTER_DEVICE_PADDLE/SPORTS_PAD/MD_PAD`; append axis fields to `cogmaster_input`. | `--port1/--port2` accept the new names; `--input` line gains an optional axis suffix. Adapter: `SET_CONTROLLER_INFO` subclasses, left stick to axes. |
| **Light Phaser** | New catch-up call sites in `bus_in`/`bus_out` for `$7E/$7F`, `$DC/$DD/$C0-$FF`, `$3F`; one extra call at the VDP line-512 event and at frame end; each guarded by one flag test. Sampling reads `field[y][x]` after `render_until`. | `lphaser` state: sampler cursor `at`, lit, sticky seen, plus `io.th_eff`; cross-check `at <= ticks` and within one frame, like `psg_io`. | Device constant `LIGHT_PHASER`; axes `x,y` in `cogmaster_input`. | Adapter maps `RETRO_DEVICE_LIGHTGUN` screen coordinates to field coordinates (crop-aware); crosshair is host-drawn. Runner: `--port1 lightphaser` plus axes in `--input`. Not available on native GG (no TH pins). |
| **3-D glasses** | `bus_write` for `$FFF8-$FFFB` calls `io_snoop()` (the write also lands in RAM as today). Not routed through `mapper_write`, which would misfire the Sega RAM-window branch. | One byte of eye state plus a write counter in `cogmaster__io`. | Append `cogmaster_run_result.glasses` (0 never written, 1 right open, 2 left open). Output struct, older callers keep their layout. | Adapter option `cogmaster_3d_glasses` (both, left, right, anaglyph) composes from the flag with one frame of memory. Frame hash stays per rendered frame. |
| **BIOS path** | `$3E` b3 (active low) maps the BIOS at `$0000-$1FFF` (SMS, exactly 8,192 B) or `$0000-$03FF` (GG, 1,024 B); `mem_driven` and `cogmaster_peek` share the decode. `boot_power_on()` replaces the hand-off: Z80 reset state, VDP registers zero, `$3E` at its power-on value. | BIOS bytes are media, not state (as the ROM). Header gets `bios_crc32` (4 B) and `boot` (1 B) from the 11 reserved bytes, so a state loads only into the same boot configuration. | `cogmaster_media` appends `bios`, `bios_size`, `boot`; status `ERR_BIOS_REJECTED`; `cogmaster_info` appends `boot` and `bios_known`. | Runner `--boot fast|bios --bios FILE` (exit 77 when BIOS required but absent), prints SHA-256. Adapter reads the file itself from `GET_SYSTEM_DIRECTORY` (host I/O, not core), `.info` firmware rows, option `cogmaster_boot`. |
| **PAL, Japanese FM over the ledger** | Nothing new: `tv`, `region`, `fm` profile fields, PAL V tables, PAL audio rationals and the OPLL unit are shipped. | none | none | Work is ledger configuration dimensions (`tv=pal`, `fm=on`), evidence rows, and the rcheevos disallowed-option note for `cogmaster_region = pal`. |
| **Ledger over the CC0 lists; drop `is_experimental`** | none | none | none | `src/cartdb.inc` (1,657 rows) is the row source; `tests/compat` and `scripts/compat-*.sh` extend; the `.info` flag guard lives in `tests/cmake/repository.sh`. |

## 4. Pin-level port model (the refactor that peripherals share)

Today the pads are inline in `io.c`. Three devices need TH as an output or an asynchronous input, so introduce one boundary before adding any of them.

```c
/* ports.c: levels the device drives at `tick`, bit0 Up .. bit3 Right, bit4 TL, bit5 TR, bit6 TH.
   1 = high. A pin the console drives as an output ($3F) is overridden by io.c, not here. */
uint8_t cogmaster__ports_pins(cogmaster *m, uint32_t port, uint64_t tick);
void    cogmaster__ports_th_edge(cogmaster *m, uint32_t port, uint8_t level, uint64_t tick);
```

- `io.c` mixes device pins with `$3F` directions and levels, the Japanese output-echo-low rule, the SMS1 reset line and `$3E` b2. That mixing code stays where it is and keeps its tests (`io.pad_pins`, `io.port3f_region`, `io.port_dc_dd`).
- **One TH edge detector.** Store the effective TH pair (`io.th_eff`, 2 bits) in state. Every change to either TH (a `$3F` write, a gun transition, a sports-pad output change) goes through one `th_changed(old, new, tick)` in `io.c`, which applies the VDP latch rule. Today's rule (latch on low-to-high on either TH) moves into this function unchanged, so existing hashes cannot move.
- **Sports Pad** is a four-phase nibble reader clocked by TH edges: X high, X low, Y high, Y low, with a timeout that resets the phase. Timeouts are tick counts (60 µs is about 644 ticks, 143 µs about 1,536; use NTSC ticks on both TVs and say so). Per-frame input is a signed **delta** that the device accumulates until the next sequence start, then clamps to a signed byte; the accumulator is state.
- **Paddle** is absolute (one byte). Its nibble toggles on a fixed tick divider (ares measured about 18 kHz; choose an integer divider near 596 ticks per cycle). Stateless apart from the position.
- **Device change** (`port1/port2` in `cogmaster_input`) resets that port's device state when it applies at the frame boundary, deterministically.
- Buttons ride the existing `p1/p2` bits (TL = button 1, TR = button 2), so only axes are appended to the input struct.

## 5. What the Light Phaser needs from the VDP's per-line model

The per-line model is sufficient. No per-dot fetch machine is needed, because the renderer already emits pixels at dot granularity into `field[][]` and the cursor (`render_line`, `render_x`) is monotonic. The gun consumes only **emitted** pixels, which are immutable once written. Five concrete needs:

| Need | Today | Change |
|---|---|---|
| Emitted pixel at (x, y) up to the beam | `field[y][x]` after `cogmaster__vdp_render_until(line, x)` | none; call it with the **strictly-before** rule (dots `< tick/2`), the rule writes use. A status read includes its own dot; the gun must not, or it could render a dot a same-tick VRAM write should still change, and frame hashes would depend on whether a gun is plugged in |
| H counter at an arbitrary tick | `vdp_hcount()` reads `m->bus.t` | add `vdp_hcount_at(t)` and `vdp_latch_h_at(t)`; the existing latch becomes the `t = bus.t` case |
| Latch edge policy | rising edge of effective TH, in `io_control` | make it one function (`th_changed`) shared by `$3F` and gun transitions; keep the per-title delay as one named constant plus an optional small facts table |
| Catch-up never lags past a repaint | the renderer overwrites row `y` again one frame later | gun catch-up also runs at the VDP's tick-512 line event and at frame end, so it never samples a row after the next field has begun overwriting it |
| Active area only | `line < height`, dots 0-255 | clip the sensor window to the active field; the borders emit nothing |

**Sensor model (selected, confidence MEDIUM, settled in the phase).** From [PREP-CART] section 3.3: radius 6, polled every 4 dots along the beam, lit when the emitted dot's luma is at or above a threshold, sticky "seen" flag cleared on a `$DD` read or at frame start (two titles need it). Implement as a pure sampling grid:

```c
/* lphaser.c: lazy like psg_catch_up. Only lines |y - cy| <= 6 and dots inside the disc are visited,
   at most about 50 samples per frame, so the cost is independent of how often the game polls. */
void cogmaster__lphaser_catch_up(cogmaster *m, uint64_t tick)
{
    struct cogmaster__gun *g = &m->io.gun;           /* cx, cy latched at frame start; at; lit; seen */
    for (sample s = next_sample(g); sample_tick(s) + 2u <= tick; s = next_sample(g)) {
        cogmaster__vdp_render_until(m, s.line, s.dot + 1u);  /* dots < tick/2 holds: s.dot + 1 <= tick/2 */
        uint8_t lit = in_disc(g, s) && luma444(m->field[s.line][s.dot]) >= GUN_LUMA;
        g->at = sample_tick(s);
        if (lit != g->lit) { g->lit = lit; cogmaster__io_th_changed(m, gun_th_level(lit), g->at); }
    }
}
```

`luma444` is an integer table lookup over the 4,096 RGB444 values (const data in `.rodata`, allowed). The threshold is the only number MAME's float luma forces us to re-derive, so it is a named constant with a test.

**Hazards.** (1) The TH edge's H value must be taken at the **edge tick**, not the CPU's read tick; this is why `vdp_hcount_at` is mandatory. (2) The H counter's tick-to-counter translation is a documented coordinate inference ([VIDEO]: 32-tick origin shift, counts of 2 dots); the gun turns that inference into an aiming offset the player sees, which is why MAME carries per-game offsets. Do not bake per-game constants into the VDP; keep one global delay and a facts table keyed by SHA-1, filled only from evidence. (3) Hash neutrality: with no gun connected the new calls are one flag test each and change nothing; with a gun, frames still depend only on inputs and ticks. A fixture hash with the gun connected and the cursor off-screen must equal the pad-only hash.

## 6. Data-flow changes

**Input.** v1.0: `set_input` stores a pending absolute record; `run_frame` latches it at the frame boundary and computes the Pause edge; `io.c` reads `p1/p2`. v2.0: the record also carries axes and device kinds; `frame.c` applies them and resets any device whose kind changed; `ports.c` turns record plus tick into pins; `io.c` mixes pins and `$3F`; the gun adds time-driven TH edges that bypass `$3F`; `th_changed` is the only place a latch fires.

**Saves.** Unchanged shape, one more source. EEPROM word writes call `cogmaster__battery_write(m, offset, byte)` for the changed bytes, which already bumps `generation` for offsets inside the classified span. The host sees a 128 B span and a generation exactly as it sees 8/16/32 KiB of Sega RAM. Keep the EEPROM image in `cart.ram`, not in a private buffer, or `get_battery`, `import_battery`, the runner and libretro each need a special case.

**Boot.** FAST (default): `boot_handoff()` as today. BIOS: `load` copies the bytes (allocator, like the ROM), identifies them by CRC-32 (facts only), `boot_power_on()` sets Z80 reset state (PC 0), VDP registers zero (no `$0E`/`$FF`), `ram_fill`, `$3E` at its power-on value, and the beam at the same frame origin. `bus.c` serves the BIOS window while `$3E` b3 is clear. A BIOS that is requested but absent falls back to FAST with a flag in `cogmaster_info`, never a core status (the adapter and runner decide to warn or exit 77).

**Glasses.** Write at `$FFF8-$FFFB` sets eye state; `run_frame` publishes it in `run_result`; the adapter composes. The core never changes pixels.

## 7. Patterns to follow

### Pattern 1: new boards are new `switch` arms in a new file, existing arms untouched
**What:** `mapper.c` keeps Sega, Codemasters, ROM-only byte for byte; each new board is a `case` that calls a function in `mapper_ext.c`. **Why:** the proven read path is hash-pinned on eight lanes and under the Ir/frame gate; a rewrite into a window table buys nothing the Korean 29 of 33 titles need. **When to revisit:** only if the armed Ir gate shows more than 1 percent on `boot_to_play` after the new arms. A `const` table of function pointers fails `abi.globals` (lesson XFER-071), so the `switch` stays.

### Pattern 2: one precomputed `slow` flag on the bus read path
**What:** today `mem_read` tests `mem_ctrl & (cart_off | ram_off)` per read. Replace with `bus.slow`, recomputed on `$3E` writes, at load and in state APPLY (it is derived, so a load recomputes it deterministically), covering cart off, RAM off and BIOS on. **Why:** BIOS decode must add no branch to the common case, and the perf gate is about to be armed.

### Pattern 3: devices that depend on beam position are lazy, bounded and observed at three points
**What:** every beam-dependent device keeps `at` and catches up (a) before any access that can observe it, (b) at a dispatcher event that prevents it lagging past a repaint, (c) at frame end. State validation bounds `ticks - at` as `psg_io` does. **Example:** the gun above; the PSG and OPLL already follow it.

### Pattern 4: append-only structs with a zero-defaulting copy
```c
/* set_input: copy only what the caller's tag covers, zero the rest (fixes the over-read in section 1). */
uint32_t n = tag < (uint32_t)sizeof *in ? tag : (uint32_t)sizeof *in;
memset(&m->pending_input, 0, sizeof m->pending_input);
memcpy(&m->pending_input, in, n);
```
New media fields follow the existing `reserved` pad macros under `UINTPTR_MAX` and a `_Static_assert(offsetof(...))` pin for both pointer widths. A pointer read at a tagged offset needs a `memcpy`-based accessor next to `tagged_field()`, which reads `uint32_t` only.

### Pattern 5: a state field has a bound and, where possible, a cross-check
New fields join `state_io` with min and max, and `VALIDATE` checks relations the way `bus_io` and `psg_io` do (`at <= ticks`, bit count within the opcode length, `th_eff` equal to the pins implied by `io_ctrl` and the device when no gun is connected). The `RECORD` sweep then rejects out-of-range values automatically. Update the `_Static_assert(sizeof(...))` tripwires deliberately in the same change; they exist to fail the build when a field is forgotten.

## 8. Anti-patterns to avoid

| Anti-pattern | Why bad | Instead |
|---|---|---|
| Routing `$FFF0-$FFFB` writes into `mapper_write` | The Sega arm treats `addr >= $8000` with RAM enabled as a cart-RAM write and would corrupt it | Give glasses their own `io_snoop`; give Seo Jin a board-checked early return, or defer Seo Jin (1 title) |
| Eager per-dot gun events on every line | Cost proportional to frame size for a device used by 7 titles | Lazy sampler over the disc only |
| Letting the gun render through its own dot | Frames would differ by whether a gun is attached | Strictly-before rule, with a plugged-versus-unplugged hash equality test |
| EEPROM in a private buffer | Breaks battery span, generation, import, runner, libretro | Image in `cart.ram` in `.srm` byte order |
| Sports pad or paddle timing from any host clock | Breaks determinism (`hygiene` symbol test) | Ticks only; paddle clock as `ticks / divider` |
| Making BIOS bytes state | State size varies with a user file; leaks content | Media, with a CRC in the header |
| Testing the BIOS path against a real dump in CI | No BIOS bytes may enter the tree and tests may not self-skip | A self-authored mini BIOS in WLA-DX assembly for CI; the real dump runs only through the private `library` preset, run by the agent locally |
| Per-game gun offsets inside `vdp.c` | Hides a model error behind a table | One global constant, facts table keyed by SHA-1, each row backed by evidence |

## 9. State, API and behaviour impact (per phase)

API grows by one with every header addition [PREP-ARCH section 4]; ABI stays 1 because only size-tagged structs and constants are appended. State format is internal until v3.0.

| Phase | API | State format | Behaviour revision | State changes |
|---|---|---|---|---|
| Mappers | 9 to 10 (board constants) | 11 to 12 | unchanged: no existing frame or audio hash moves | `mapper` struct, per-board bounds |
| Saves (EEPROM) | 10 (none needed) or 11 if a board constant is added | 12 to 13 | unchanged | `eeprom` struct, geometry 128 B |
| Peripherals | to 11 or 12 (input axes, device constants, `run_result.glasses`) | 13 to 14 | unchanged; pad-only content hashes must stay equal | `io.th_eff`, port device state, glasses, gun |
| BIOS | next (media fields, status, info) | 14 to 15 | unchanged for FAST; BIOS boot is a new input, not a change | header `bios_crc32` + `boot`, power-on `$3E` |
| PAL/FM ledger | none | none | unchanged | none |

Bump the behaviour revision only if an existing fixture's frame, transition or PCM hash moves; none is expected, and the eight-lane `hash-equality` job is the proof. Check `tests/cmake/hashes.cmake` before assuming a state hash is a compared artifact; it currently carries frame hashes and fixture digests.

## 10. Cost and bounds

| Addition | Per-call bound | Per-frame bound | Notes |
|---|---|---|---|
| Korean arm | O(1) | O(accesses) | one extra `switch` arm; Janggun reverse and Seo Jin XOR are per-read bit ops |
| EEPROM | O(1) per `$8000` write | none | at most 25 clocked bits per command; unknown opcode ignored |
| Pad devices | O(1) | none | sports pad phase and timeout from ticks |
| Light gun | at most 3 samples per observation after the first of a line | about 50 samples | independent of poll rate |
| BIOS | none after load | none | boot costs more emulated frames, never unbounded: every call still returns at the frame deadline |
| `slow` flag | one compare | | replaces an existing masked test |

Zero allocation after load still holds: BIOS and EEPROM memory is taken in `load` (the BIOS buffer in the same allocator call pattern as the ROM, freed in `cart_install`-style teardown).

## 11. Suggested build order

Dependencies drive it; each phase ends in a release (AGENTS rule 10). The tune-up comes first because the perf gate must be armed **before** the bus and I/O hot paths grow branches.

1. **Tune-up (SEED-007).** Prerequisite, not architecture: arm Ir/frame so every later phase is measured. Also decide the "runner `--save-dir`" requirement text (section 1, row 2).
2. **Mappers.** Needs only `cart.c`, the generator and `mapper_ext.c`; introduces the grown `cogmaster__mapper` and `guess_board()`. Order inside: `korean_nb` to ROM-only (generator change, no code), Korean `$A000`, Hicom, Zemina plus Nemesis (these cover 29 of the 33 Korean-family SMS entries); then Janggun and 4PAK if cheap; 188-in-1 and Seo Jin last (their menu entries fail in MAME and ares too). Evidence: WLA-DX register fixtures per board plus the ledger rows.
3. **Saves.** Depends on the mapper split (EEPROM is a Sega variant in `mapper_ext.c`). Adds `eeprom.c`, generator geometry, `.srm` byte order from the released libretro binaries run once, `--save-interval`, `libretro_saves = "true"`, hosted save round trip (the existing 300-line e2e budget has one slot for it).
4. **Pad devices and 3-D glasses.** Introduces `ports.c`, `th_changed`, the input append (with the `set_input` copy fix), `--port` names, libretro subclasses, `io_snoop`. This is the refactor the gun depends on, and it ships value alone (Sports Pad titles, Paddle titles, 3-D titles).
5. **Light Phaser.** Depends on step 4 and on `vdp_hcount_at`. Highest risk, so it ships alone with a self-authored fixture that lights a known square, polls TH, and reads H and V; compared against released MAME and ares binaries as oracles (binaries only, never source), plus the owner's hardware run if the offset cannot be settled.
6. **BIOS boot.** Independent of 4 and 5 except for API numbering and `bus.c` conflicts; placed after them so `$3E` power-on and the `slow` flag are already proven. Fixture: a self-authored 8 KiB mini BIOS that runs from `$0000`, writes `$3E`, copies to RAM, hands off to a cartridge and checks the power-on state.
7. **PAL, Japanese FM and whole-list ledger.** Last, because ledger statuses depend on every board and device existing; then remove `is_experimental` behind the guard test.

Within a phase, shared-inventory plans stay sequential and disjoint files (`eeprom.c` versus runner flags) run as parallel waves [PREP-ARCH section 11].

## 12. Open decisions that need phase-level research

| Decision | Position now | Settle by | Phase |
|---|---|---|---|
| TH latch edge for a gun (rising edge as shipped, or falling, or active level) and the per-title delay | Keep the shipped rising-edge rule in `th_changed`; one global delay constant | Self-authored fixture, released-binary comparison, "Color and Switch Test" and "Hang-On and Safari Hunt" behaviours [PREP-CART section 8] | Light Phaser |
| Sensor rule details (disc test, luma threshold, sticky flag lifetime) | Section 5 model | MAME's `lphaser.cpp` is BSD-3 and may be read at its pin; confirm against the seven titles | Light Phaser |
| `$3E` power-on value and who drives `$0000` when BIOS and cartridge are both enabled (MAME ANDs outputs, ares lets the cartridge override) | BIOS wins at its window while b3 is clear; `$3E` power-on documented as "undefined" [PREP-SYS section 7] | Self-authored mini BIOS plus the real dump through the private preset; both are facts-only | BIOS |
| EEPROM `.srm` word order and any busy period on `DO` | Image stored in `.srm` order; instant completion | Run released GPGX and Gearsystem binaries once on a 93C46 title and diff the bytes [PREP-CART section 8] | Saves |
| Seo Jin and 188-in-1: worth supporting | Defer both unless free | Ledger count (1 title each) | Mappers |
| Mark III (`COGMASTER_MACHINE_MARK3`) | Not in v2.0 scope; the media enum value and `io_decode` seam stay reserved | Owner scope | later |

## Sources

- `include/cogmaster.h` (API 9, behaviour 11, state 11), `src/internal.h`, `src/bus.c`, `src/io.c`, `src/mapper.c`, `src/battery.c`, `src/boot.c`, `src/cart.c`, `src/cartdb.c`, `src/instance.c`, `src/vdp.c`, `src/vdp_render.c`, `src/frame.c`, `src/state.c`, `tools/cartdb.c`, `runner/save.c`, `runner/input.c`, `runner/args.c`, `libretro/libretro.c`, `libretro/options.h`, `libretro/cogmaster_libretro.info.in` at `b9c6eb7` (HIGH: read directly).
- `.planning/preparation/ARCHITECTURE.md`, `HOSTS-AND-INTEGRATION.md`, `SMS-CARTRIDGES-AND-PERIPHERALS.md`, `SMS-SYSTEM.md`, `SMS-VDP.md`, `docs/hardware/video.md`, `docs/hardware/cartridges.md`, `.planning/PROJECT.md`, `.planning/seeds/SEED-001-whole-library.md`, `.planning/LESSONS.md` (HIGH for project facts; hardware claims keep the confidence the preparation files gave them: MEDIUM where they are MAME or ares behaviour, LOW where marked unverified there).
- Library shares (29 of 33 Korean-family SMS entries are Hicom 9, Zemina 8, Korean 6, korean_nb 5, Nemesis 1) are from [PREP-CART] section 2.3, computed from the CC0 lists at MAME `1d7a454ed79e`; re-run its count command against `src/cartdb.inc` when planning the mappers phase.
