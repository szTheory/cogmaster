---
title: "Architecture: time model, modules, public API, I/O contracts and build order"
summary: "The technical shape of the cogmaster core: one VDP-clock timeline driven by the Z80's time-stamped bus accesses, lazy devices behind one deadline dispatcher, the module and seed map, the public C API and its versions, the frame, audio, input, determinism, state, observability and boot contracts, the v1.0 build order, and how each research conflict is settled."
read_when: "Before planning or writing core, runner or libretro code; before adding a module, API call, profile field or hash; whenever two preparation files disagree."
updated: 2026-10-18
---

# cogmaster architecture

## Where to look
| Question | Section |
|---|---|
| What ships, and what does the core never do? | 1 |
| What is a tick, who advances time, how are interrupts and frames bounded? | 2 |
| Which module owns what, seeded from where, landing when? | 3 |
| Which public functions, struct rules and version numbers? | 4 |
| Video, audio and input formats and hashes | 5 |
| Determinism rules, profile fields, hostile inputs | 6 |
| Save states, battery media | 7 |
| Counters, inspection, first fault, hooks | 8 |
| Fast boot, BIOS boot, test card | 9 |
| Seams kept for later features | 10 |
| v1.0 build order | 11 |
| Portability and performance | 12 |
| How each research conflict is settled | 13 |

Citations: [ARCH.01] SMS-SYSTEM, [ARCH.02] SMS-VDP, [ARCH.03] SMS-AUDIO, [ARCH.04] SMS-CARTRIDGES-AND-PERIPHERALS, [ARCH.05] HOSTS-AND-INTEGRATION, [ARCH.06] ENGINEERING, [ARCH.07] LESSONS; "§n" and "Cn" are that file's sections and choices. Choices read **Rec**. **Alt** (strongest alternative). **Flip** (would change if). Internal names below drop the `cogmaster__<module>_` prefix [ARCH.06 §10]. Timing coordinates: line n, tick t in 0–683, tick 0 = first active pixel [ARCH.01 §2].

## 1. Shape
| Deliverable (directory) | What it is | Built from |
|---|---|---|
| Core library (`include/cogmaster.h`, `src/`) | one terse public header (≤ 24 KB) and one module per chip (§3); external symbols `cogmaster_*`, cross-file internals `cogmaster__*` | C17; links nothing beyond `memcpy memmove memset memcmp` [ARCH.06 §2] |
| `cogmaster-run` (`runner/`) | headless: `--probe`, frame/audio/state hashes, `--png`, `--bench`, `--trace first-fault` from phase 2, later saves, movies, `--vgm`; one exit-status table [ARCH.05 §4] | core + host helpers |
| `cogmaster_libretro` (`libretro/`) | the sibling's ~350-line adapter shape: one instance, test card without content, XRGB8888 (RGB565 under `USE_RGB565`), stereo batch audio, options, memory exports [ARCH.05 §1] | core + host helpers + vendored `libretro.h` |
| Host helpers (`host/`), outside the determinism boundary | `palette.c` maps RGB444 to RGB888 through the SMS1 / SMS2 / GG level tables (MAME BSD-3 values with notice) [ARCH.02 §6]; later `lcd.c` (GG ghosting, opt-in) [ARCH.05 §7]; never hashed | runner and adapter |

The core never opens a window, an audio or input device or a file, reads a clock or the environment, starts threads, keeps mutable globals, uses floating point (the adapter's aspect ratio is the only float, outside the core), allocates after `cogmaster_load`, prints, aborts or exits [ARCH.06 §2, §10][ARCH.07 XFER-070]. Bytes come in; caller-owned buffers go out.

## 2. Time and stepping
| Constant | NTSC SMS, Game Gear | PAL SMS | Source |
|---|---|---|---|
| Tick = one VDP clock (crystal/5; GG crystal/3) | 10,738,635 Hz | 10,640,684.8 Hz | [ARCH.01 C1] |
| Z80 T-state = 3 ticks; pixel = 2; PSG step = 48; YM2413 sample = 216 | 3,579,545 Hz | 3,546,895 Hz | [ARCH.01 §1][ARCH.03 §1] |
| Line = 684 ticks = 342 pixels = 228 T-states | 15,699.76 Hz | 15,556.56 Hz | [ARCH.01 §1] |
| Frame = 262 / 313 lines | 179,208 ticks, 1,640,625/27,379 Hz (= (315,000,000/88)/59,736) | 214,092 ticks, 13,300,856/267,615 Hz | [ARCH.01 §1][ARCH.05 §1] |

| Tick in line | Event fired by the dispatcher (one table in `vdp.c`) | Source |
|---|---|---|
| 0 | line start: the line renderer may flush the previous line; V counter line advance for rendering | [ARCH.02 §4.2] |
| 558 | HSYNC falls (statistics only in v1) | [ARCH.01 §2] |
| 600 | `VDP_LATCH_TICK`: latch reg 8, reg 1 display enable and the next line's sprite scan | [ARCH.02 §4.9] |
| 604 | `VDP_VCOUNT_TICK`: CPU-visible V counter steps (reads use the §4.4 tables) | [ARCH.02 §4.9] |
| 607 | `VDP_INT_TICK`: frame-INT pending on the last active line (192/224/240); line counter test and line-INT pending at 608, one T-state later | [ARCH.01 §2][ARCH.02 §4.5–4.6] |
| 611 | HSYNC rises; on line 260 (NTSC) or 311 (PAL), the start of the last frame line, the latched Pause press asserts /NMI (the logic analyser's "/HSYNC edge 611 mclk into line 260"; a handler's `IN A,($7E)` then reads $FF, where one line later it would read $00) | [ARCH.01 C8] |

- **Counters.** One `uint64_t ticks` per instance, plus `line` and the derived `t = ticks mod 684`. 684 = 3 × 228 and a frame is a whole number of T-states, so CPU, VDP and PSG never drift and no phase field is needed [ARCH.01 §1].
- **The Z80 drives the bus.** `z80.c` executes whole instructions as M-cycles; every memory or port access calls `bus_read/bus_write/bus_in/bus_out(addr, tick)` with the tick of its own data T-state (M1: T3 sample, memory: T3, I/O: T3 after the automatic wait) [ARCH.01 §2]. There are no wait states on either machine [ARCH.01 §2]. **Rec** instruction-granular Z80 with T-state-exact stamps [ARCH.01 C3]. **Alt** a per-T-state Z80 that stops anywhere (ares). **Flip** a conformance vector needs an interrupt or NMI taken mid-instruction (none can: the Z80 samples both at the last T-state only [ARCH.01 §2]).
- **One deadline dispatcher.** `bus.c` is the only place time advances: before serving an access at tick τ it fires, in order, every dispatcher event at or before τ from one cached `next_deadline` (line events above, frame end, audio frame mark), so the fast path is one compare. The Z80 itself advances `ticks` by 3 per T-state between accesses.
- **Lazy devices.** Each device keeps a `caught_up_to` tick and advances to τ only when an access can observe it [ARCH.02 §9][ARCH.03 §5]:

| Device | Catches up before | Cost bound per catch-up |
|---|---|---|
| VDP renderer | a CRAM, reg 0/1/7, VRAM or SAT write during an active line; frame end; `get_frame` | the dots since the last catch-up on this line (≤ 256) |
| VDP counters, status, read buffer | any $7E/$7F/$BE/$BF read | O(1): computed from (line, t) and the tables |
| PSG, YM2413 | any $40–$7F, $F0–$F2 write; frame end | the PSG steps (48 ticks) or OPLL samples (216 ticks) elapsed |
| Mapper, ports, $3E/$3F | nothing: combinational, served at τ | O(1) |
| H-counter latch | a TH edge on either port (a `$3F` write in v1; Light Phaser later) asks the VDP for the counter at τ | O(1) |
- **Interrupts.** /INT is a level: pending flags AND IE bits, held until the status port is read [ARCH.02 §2.6]; the Z80 samples it at the last T-state of each instruction, takes it only with IFF1 set, and EI defers acceptance until after the following instruction; IM 1 costs 13 T-states, IM 0/2 read the open-bus byte of C5 [ARCH.01 §2]. /NMI is an edge latched from the Pause press (GG: Start in SMS mode only) and asserted at tick 611 of line 260 NTSC / 311 PAL, the start of the last line, as the logic analyser measured [ARCH.01 C8]; it costs 11 T-states. HALT executes NOPs until either is sampled [ARCH.01 §2].
- **Frames and bounds.** A frame is a fixed 179,208 / 214,092 ticks. **Rec** the frame origin is tick 607 of line 192 at power-on (the 192-line frame-INT tick, inside blanking for every height) and never moves; `run_frame` stops at the first instruction boundary at or after the frame-end tick, and the overshoot (≤ 23 T-states = 69 ticks, so never past tick 676 of the origin line [ARCH.10]) is charged to the next frame, keeping cumulative ticks exact. `set_input` takes effect at that boundary.. **Alt** a boundary that follows the current height's frame-INT (variable frame length on a mode change). **Flip** a TAS consumer needs frames aligned to the 224-line VBlank. Nothing a ROM does stops time: HALT and DI loops advance ticks, a VDP hammer costs one catch-up per access (bounded by dots per line), a loaded state's ticks are capped at 2^62. Stop reasons: `FRAME` (v1, always), `BUDGET` and `HOOK` (v5 `cogmaster_run`).

## 3. Modules
| Module | Owns | Interface | Seed or own | Lands |
|---|---|---|---|---|
| `z80.c` | registers incl. WZ, Q, R, IFF1/2, IM, pending-EI, HALT; all documented and undocumented opcodes; flag tables; interrupt acceptance | `z80_step()`; memory and ports only through `bus_*`; `z80_int_level()/nmi_edge()` polled from bus state | own, cycle table from the Zilog manual, verified against SingleStepTests/z80 (MIT); ares `component/processor/z80` (ISC) read for structure only [ARCH.07 XFER-005]; seed notices per PROVENANCE, pending [ARCH.11] | P2 |
| `bus.c` | `ticks`, decode (A15/A14 RAM, $3E slot enables, A7/A6/A0 port decode and the full-decode profile), open-bus byte, deadline dispatcher, lag credit | `bus_read/write/in/out(addr, tick)`, `bus_peek` | nesturbator bus pattern | P2 |
| `mapper.c` | Sega ($FFFC–$FFFF, RAM, bank shift, b7), none, Codemasters (+8 KB RAM); `switch (board)` dispatch | `mapper_read/write`, `mapper_peek` | own from [ARCH.04 §2] | P2 (Sega, none, Codemasters); Korean, MSX, Janggun, 93C46 v2 |
| `vdp.c` | 315-5246 default, 315-5124 and GG profiles; regs, control port, read buffer, status, line counter, H latch, line renderer, sprite scan, collision, field buffer | `vdp_read/write(port, tick)`, `vdp_render_to(tick)`, `vdp_counter(tick)` | own from [ARCH.02]; MAME `315_5124.cpp` (BSD-3) cross-check only | P3 |
| `psg.c` | SN76489 variant: 16-bit LFSR, taps 0/3, period 0 = 1, 2 dB table, GG $06 mask | `psg_write(v, tick)`, `psg_catch_up(tick)` | own from [ARCH.03 §2–3]; MAME `sn76496.cpp` cross-check | P1 silence, P4 |
| `opll.c` | YM2413, $F0/$F1/$F2 per `fm_unit` | `opll_write(a, v, tick)`, `opll_catch_up(tick)` | C port of ymfm OPLL (BSD-3) with notice; emu2413 table beside it [ARCH.03 FM-CORE] | P4 |
| `io.c` | ports $3E/$3F/$DC/$DD (+mirrors), GG $00–$06, region/TV readback, pad devices at pin level, TH latch request | `io_in/out(port, tick)`, `io_pins(port, tick)` | own from [ARCH.01 §4][ARCH.04 §3] | P2 ports (SMS and GG $00–$06), P3 pads |
| `cart.c` + generated `cartdb.c` | copier-header strip, SHA-1 table from the three CC0 softlists, TMR SEGA probe, opcode scan, board choice | `cart_probe()`, `cart_load()` | own; table generated from MAME `hash/*.xml` (CC0) [ARCH.04 §1.4] | P2 probe, P3 table |
| `boot.c` | fast-boot hand-off (C7), BIOS identification by digest and mapping under $3E | `boot_fast()`, `boot_bios_identify()` | own: facts only, no BIOS bytes [ARCH.01 §6] | P2 fast, v2 BIOS |
| `synth.c` | per-cycle level merge, 16-tap × 32-phase kernel, DC blocker, stereo rings, phase accumulator | `synth_level(tick, source, level)`, `synth_render()` | nesturbator `src/synth.c` (MIT, owner) [ARCH.03 SYN] | P1 silence, P4 |
| `state.c` | visitor and 64-byte header (§7) | `state_visit(mode)` | nesturbator `src/state.c` pattern [ARCH.08] | visitor P1, format v3 |
| `instance.c`, `testcard.c` | public API, config, profile, frame driver, counters, first fault, test cards | `include/cogmaster.h` | nesturbator pattern | P1 |
| `host/palette.c` | RGB444 → RGB888 level tables | `host/cogmaster_host.h` | MAME levels (BSD-3) [ARCH.02 §6.1] | P1 |
| `runner/`, `libretro/` | §1 | none | nesturbator/muhtari adapter shape [ARCH.05 §1, §4] | P1 |

Dependency direction, no cycles: `instance → z80 → bus → {vdp, psg, opll, io, mapper, cart RAM}`; `psg`/`opll → synth`; `vdp` never calls the bus or the CPU; interrupt lines are state the Z80 reads through `bus`. Boards dispatch through one `switch`: a `const` table of function pointers lands in `.data.rel.ro` under PIC and fails `abi.globals` [ARCH.07 XFER-071][ARCH.06 §2].

## 4. Public C API
```c
cogmaster_status cogmaster_get_versions(cogmaster_versions *out);       /* library, API, ABI, behaviour, state, battery */
cogmaster_status cogmaster_create(const cogmaster_config *cfg, cogmaster **out); /* allocator hook, audio rate num/den */
void             cogmaster_destroy(cogmaster *m);
cogmaster_status cogmaster_probe(const void *image, size_t n, cogmaster_cart_info *out); /* pure: header, hashes, guesses */
cogmaster_status cogmaster_load(cogmaster *m, const cogmaster_media *media); /* image, BIOS or NULL, profile; bytes copied */
cogmaster_status cogmaster_reset(cogmaster *m, uint32_t kind);           /* SOFT: hand-off again; POWER: + ram_fill, VRAM/CRAM */
cogmaster_status cogmaster_set_input(cogmaster *m, const cogmaster_input *in); /* latched at the next frame start */
cogmaster_status cogmaster_run_frame(cogmaster *m, cogmaster_run_result *out); /* one frame of ticks; stop reason, overshoot */
cogmaster_status cogmaster_get_frame(const cogmaster *m, cogmaster_frame *io); /* copies crop or field into io->pixels */
cogmaster_status cogmaster_get_audio(cogmaster *m, cogmaster_audio *io);  /* drains the frame's int16 samples */
cogmaster_status cogmaster_get_info(const cogmaster *m, cogmaster_info *out); /* geometry, fps and rate num/den, PAR, profile */
cogmaster_status cogmaster_get_memory(const cogmaster *m, uint32_t kind, cogmaster_span *out); /* RAM, VRAM, CRAM, CART_RAM, BATTERY */
uint64_t         cogmaster_save_generation(const cogmaster *m);
size_t           cogmaster_state_size(const cogmaster *m);              /* constant from load to unload */
cogmaster_status cogmaster_save_state(const cogmaster *m, void *buf, size_t n);
cogmaster_status cogmaster_load_state(cogmaster *m, const void *buf, size_t n); /* validated in full before any byte applies */
uint8_t          cogmaster_peek(const cogmaster *m, uint32_t space, uint32_t addr); /* BUS, RAM, VRAM, CRAM, CART_RAM; no side effects */
cogmaster_status cogmaster_poke(cogmaster *m, uint32_t space, uint32_t addr, uint8_t v); /* RAM, VRAM, CRAM, CART_RAM only */
cogmaster_status cogmaster_get_z80(const cogmaster *m, cogmaster_z80_view *out);
cogmaster_status cogmaster_get_vdp(const cogmaster *m, cogmaster_vdp_view *out); /* R0–R10, address, code, buffer, status, line, t */
cogmaster_status cogmaster_get_counters(const cogmaster *m, cogmaster_counters *out);
cogmaster_status cogmaster_get_fault(const cogmaster *m, cogmaster_fault *out); /* first fault since reset, if any */
cogmaster_status cogmaster_set_hooks(cogmaster *m, const cogmaster_hooks *h); /* v5 */
cogmaster_status cogmaster_run(cogmaster *m, const cogmaster_budget *b, cogmaster_stop *why); /* v5: bounded in ticks */
```
Boundary structs that the contracts in §5 and §8 refer to (all size-tagged; field order is the ABI):
```c
typedef struct { uint32_t size; uint16_t *pixels; uint32_t pitch_bytes; uint32_t flags; /* CROP (default) | FULL_FIELD */
                 uint32_t x, y, w, h; uint32_t field_h; uint32_t backdrop_rgb444; } cogmaster_frame;
typedef struct { uint32_t size; int16_t *samples; uint32_t capacity; uint32_t written; uint32_t channels; } cogmaster_audio;
typedef struct { uint32_t size; uint32_t stop; /* FRAME | BUDGET | HOOK */ uint32_t overshoot_ticks; uint32_t lag; } cogmaster_run_result;
typedef struct { uint32_t size; uint32_t present; uint64_t frame; uint32_t line, dot; uint16_t pc, addr;
                 uint8_t opcode, width, device, kind; } cogmaster_fault;
```
`cogmaster_media` carries the profile: `machine` {SMS1, SMS2, GG, MARK3 (v2)}, `region` {JAPAN, EXPORT}, `tv` {NTSC, PAL}, `fm` {AUTO, OFF, ON}, `boot` {FAST, BIOS}, `ram_fill` {ZERO, SEEDED + u64}, `gg_mode` {AUTO (hash table pin 42), GG, SMS}; `tv = PAL` with `GG` or `JAPAN` is rejected [ARCH.01 C6][ARCH.04 §3.2]. `cogmaster_input`: two pads (bits as $DC/$DD), `pause` (edge-detected by the core), `gg_start`, `reset_button` (SMS1 only), reserved axes for v2 devices.

| Rule | Detail (sibling conventions [ARCH.08]) |
|---|---|
| Size tag | every boundary struct starts with `uint32_t size`; input structs: smaller than the struct's first released size → `COGMASTER_STATUS_ERR_STRUCT_SIZE`, between first released and current → missing fields default to zero, larger accepted only if the extra bytes are zero, and a total size above 4096 is `COGMASTER_STATUS_ERR_STRUCT_SIZE` whatever the bytes hold, because the zero check on the extra bytes would otherwise read an unbounded distance past the caller's object (AGENTS rule 4; a deliberate narrowing of phase 1 CONTEXT D-11, recorded in plan 01-01 and `docs/embedding.md`); output structs: the library writes min(size, its own) (phase 1 CONTEXT D-11) |
| Fields | fixed-width integers; enums carried as `uint32_t`; no `bool`, bit-fields or pointers into core state; rates as num/den |
| Statuses | append-only, specific (`INVALID_IMAGE`, `UNSUPPORTED_SIZE`, `UNSUPPORTED_BOARD`, `UNSUPPORTED_DEVICE`, `PROFILE_REJECTED`, `BIOS_REJECTED`, `STATE_*` …); lint flags are bits in `cogmaster_cart_info` (`HEADER_MISSING`, `CHECKSUM_BAD`, `REGION_UNKNOWN`, `GG_SMS_MODE_GUESSED`) [ARCH.04 §1] |
| Errors | validate, then apply: an error changes no state; a failed `load` keeps the previous content |
| Memory | allocator hook {alloc, free with size, user}; calls only in `create` and `load`, zero afterwards (`core.no_alloc_after_load`) [ARCH.06 §8] |
| Buffers, threads | frame, audio and state buffers are caller-owned; ROM and BIOS bytes are copied at load; an instance is single-threaded and instances share nothing |

| Number | Changes when | Read from |
|---|---|---|
| Library `x.y.z` | each release (release-please, `version.txt`) [ARCH.06 §5] | generated `COGMASTER_VERSION_*`, `get_versions` |
| `COGMASTER_API_VERSION` | +1 with every header addition; never decreases | the macro; an embedder refuses a runtime value below the one it compiled with |
| ABI (SOVERSION) | an incompatible signature or layout change; appending to a size-tagged struct is not one | soname, `get_versions` |
| Behaviour revision | any frame, audio or state hash changes for the same inputs, with a reason | state and movie headers, `--capabilities`; Playstead pins it beside the runner digest [ARCH.05 §4] |
| State format; battery layout | the state byte layout; the battery span layout | state header; `get_info`, `--capabilities` |

## 5. I/O contracts
| Stream | Contract |
|---|---|
| Video | internal field 256 × H (H = 192/224/240) of `uint16_t` RGB444 `0000BBBBGGGGRRRR` per dot: GG CRAM words as written, SMS 6-bit CRAM expanded `c \| c << 2` per channel exactly as a GG in SMS mode does (lossless, reversible); borders not emitted, backdrop index and border geometry in `cogmaster_info`. Crop rect: SMS (0, 0, 256, H); GG (48, 24, 160, 144), 224-line GG (48, 40, 160, 144); `get_frame` copies crop or full field at a caller pitch. **Frame hash** = SHA-256 over the crop rows, LE `uint16_t`. PAR 8:7 NTSC, 2950000:2128137 PAL, 6:5 GG, reported not applied [ARCH.02 5.A, 6.A][ARCH.05 §1]. **Rec** RGB444 only, no index plane in v1: one domain for SMS and GG, a GG title and the same ROM in SMS mode hash in the same space, and an index plane is not a stable identity because CRAM may change mid-frame (raster bars) [ARCH.02 §4.7]. **Alt** emit CRAM indices plus a per-frame CRAM snapshot (what a Gym `native` observation wants [ARCH.05 §5]). **Flip** the v5 Gym or ROM-hacking tools need indices; then a debug sideband plane (`get_frame` flag), never a second hash. |
| Audio out | `int16_t` at the configured rate num/den (default 48,000/1); `channels` 1 for SMS, 2 interleaved for GG; samples per frame NTSC 59,736 × 176/13,125 = **3,504,512/4,375 = 801.031…** (801 or 802), PAL 71,364 × 9,600/709,379 = **685,094,400/709,379 = 965.766…** (965 or 966; the AUD rational sits 0.02 ppm from the crystal-exact 1,605,690,000/1,662,607 [ARCH.10]); the fraction carries across frames in the phase accumulator; never padded; undrained samples are dropped and counted [ARCH.03 §1, §6, OUT] |
| Audio model | PSG four channels at the /16 step (Q15 table, period 0 = 1), GG $06 mask as its own source, YM2413 one 9-bit sample per 72 T-states; mix `psg × 1434 + fm × 4096` Q12 scaled so the largest sum cannot clip; per-cycle merge into the kernel, one DC blocker [ARCH.03 PSG-0, PSG-VOL, GG-STEREO, FM-MIX, SYN] |
| Audio hashes | **primary** = SHA-256 over LE records {u64 tick, u8 source, u16 level}, sources 0–3 PSG channels, 4 GG mask, 5 YM2413, 6 audio control; independent of kernel, gains and rate; runner hook tap only. **Secondary** PCM hash per (rate, channels, profile); clipping check before freezing digests [ARCH.03 HASH] |
| Input | `cogmaster_input` per frame, applied at the frame start (§2); pads as pin levels (0 = pressed) mixed with $3F directions, the region readback rule and $3E bit 2 [ARCH.04 §3.1]; Pause is an edge: the core latches a 0→1 press and asserts /NMI once at tick 611 of line 260 NTSC / 311 PAL, the start of the last line (logic analyser, [ARCH.01 C8]); holding it does nothing [ARCH.01 §7]. **Lag frame** = no CPU read of $DC, $DD, $C0, $C1 or GG $00 during the frame; peeks excluded [ARCH.05 §5][ARCH.04 §3.10] |
| Host presents, outside the hash | RGB444 → RGB888 level tables (SMS1, SMS2, GG linear), aspect, borders from the backdrop index, GG LCD filter, 3-D glasses eye select, crosshair [ARCH.05 §7] |

## 6. Determinism, profiles and safety
Integer-only (`hygiene.no_float`, `nofp`, `abi.undefined`); no mutable globals (`abi.globals`); byte-wise LE serialisation; no punning, bit-fields or packed structs; hashes byte-identical on six platforms plus `be32` and `wasm32` (`hash-equality`); every intentional hash change bumps the behaviour revision [ARCH.06 §2, §4, §9]. No host clock: the core reads no time source and a CTest proves the runner's symbol table has none [ARCH.07 XFER-070]. Every field below lives in the instance, the state and the movie header; BIOS bytes are an input, never a default [ARCH.01 §8].

| Profile field | Values (default first) | Source |
|---|---|---|
| `machine`, `vdp` | SMS2/315-5246; SMS1/315-5124 (mask bits, zoom rule, 192 only); GG/315-5378; MARK3 (v2) | [ARCH.01 C6][ARCH.02 1.A] |
| `region`, `tv` | EXPORT, JAPAN; NTSC, PAL (PAL only with EXPORT SMS) | [ARCH.01 §5] |
| `io_decode`, `open_bus` | partial (A7/A6/A0) on export SMS; full on JAPAN, GG; open bus = last data-bus byte on SMS1/Mark III, $FF on SMS2/GG | [ARCH.01 C5, C6] |
| `fm_unit` | none; smsj (built in, full $F2 table); mark3_unit (bit 1 forced 0) | [ARCH.03 FM-CTRL] |
| `ram_fill` | ZERO; SEEDED(u64) by splitmix64; JAPAN preset may choose $F0 | [ARCH.01 §8] |
| Power-on values | mapper 0/0/1/2; $3E $AB (GG $A8); $3F $FF; $F2 %00; GG $00–$06 = $C0 $7F $FF $00 $FF $00 $FF; PSG volumes $F, tones 0, LFSR $8000, latch register 3 [ARCH.03 §2.1]; VDP regs zero except R2 $0E, R10 $FF; beam at the frame origin, line 192 tick 607, ticks 0 (so every frame, the first included, is exactly one frame long) | [ARCH.01 §8][ARCH.03 §2] |
| `gg_mode` | per image from the hash table (pin 42), host override | [ARCH.04 §3.2] |
| `sms2_clip`, OPLL table | off; ymfm (emu2413 selectable) | [ARCH.03 PSG-VOL, FM-CORE] |

| Hostile input | Bounds [ARCH.04 §6] |
|---|---|
| ROM image | 1 KiB ≤ size ≤ 1 MiB + 512, else `UNSUPPORTED_SIZE`; strip 512 only when `size mod 16 KiB = 512`; pad to a 16 KiB multiple with $FF; bank = value mod pages; RAM index & (size − 1) |
| Header probe | read only when the slot exists (size ≥ $8000 / $4000 / $2000); nibbles outside their tables → lint, checksum skipped; probe is pure (property test) |
| Hash table | build-time sorted 20-byte keys, binary search, no run-time parsing |
| BIOS | exactly 8,192 (SMS) or 1,024 (GG) bytes; digest logged; unknown digests accepted with a warning |
| Battery bytes | exactly the board's RAM size (8/16/24/32 KiB) or 128 B EEPROM; shorter → pad $FF, longer → reject; never parsed |
| State | exact size; magic, CRC-32, versions, every field bound checked before any byte is applied; ticks ≤ 2^62; bank numbers re-masked |
| Fuzz targets | `fuzz.loader`, `fuzz.run` (random bytes, fast boot, 2 frames), `fuzz.state`, `fuzz.save`; each with a `fuzz.regress.*` replay on six legs [ARCH.06 §3] |

## 7. State and saves
- **Visitor.** One function per struct, run as SIZE, WRITE (save), VALIDATE then APPLY (load), plus RECORD, which tests use to emit every field's bounds for an automatic out-of-range rejection sweep; byte-wise LE; `_Static_assert` tripwires; VALIDATE walks everything, so APPLY cannot fail [ARCH.08]. The visitor exists from phase 1 (the test card's state round-trips); the public format freezes in v3.0.
- **Header**, 64 B: 8-byte PNG-style magic; u16 format version; u16 header size; u32 total size; u32 behaviour revision; u32 API version; u32 profile (byte 0 machine, byte 1 region, byte 2 tv, each the resolved value 1 or 2; byte 3 zero until vdp, fm_unit, io_decode, open_bus and ram_fill kind need packing there); u32 ROM CRC-32 (headerless body); u32 state CRC-32 (body); u64 frame; u64 ticks; 12 reserved zero bytes. The size is constant from load to unload (libretro serialize); VRAM 16 KiB + RAM 8 KiB + cart RAM dominate [ARCH.05 §1].

| Module | Contributes |
|---|---|
| z80 | AF BC DE HL AF' BC' DE' HL' IX IY SP PC, WZ, Q, I, R, IFF1, IFF2, IM, pending-EI flag, HALT, NMI latch |
| bus, io | ticks, line, open-bus byte, $3E, $3F, $DC/$DD pin state, TH latch flag, GG $00–$06, region/TV flags, lag flag |
| vdp | R0–R10, control-port state (first byte, two-byte flag, code, address), read buffer, status byte, line counter, H-counter latch, VRAM 16 KiB, CRAM (32 B or 64 B), sprite line buffer, latched reg 8 and display enable |
| psg, opll, synth | tone/noise registers, counters, output bits, latch, LFSR, GG mask, $F2; OPLL registers and every operator phase/envelope; synth phase accumulator, rings, DC blocker |
| mapper, cart | board kind, $FFFC–$FFFF (or Codemasters three), on-cart RAM, EEPROM state (v2), battery dirty flag |
| profile, counters, fault | every §6 field, the ram_fill seed, §8 counters, the first-fault record |

| State size (derived, bytes) | SMS, no cart RAM | SMS, 32 KiB battery | GG |
|---|---|---|---|
| header + Z80 + bus/io + counters + fault | ≈ 400 | ≈ 400 | ≈ 420 |
| VDP (VRAM 16,384 + CRAM 32/64 + regs, buffers, per-line stats ≈ 2,300) | ≈ 18,800 | ≈ 18,800 | ≈ 18,830 |
| work RAM | 8,192 | 8,192 | 8,192 |
| cart RAM | 0 | 32,768 | 0 (8,192 Codemasters) |
| PSG + OPLL + synth rings (two rings on GG) | ≈ 1,100 (+ ≈ 700 with FM) | same | ≈ 1,600 |
| total | ≈ 28.5 KiB | ≈ 60.5 KiB | ≈ 29 KiB |

- **Battery.** `get_memory(BATTERY)` is one span fixed from load to unload, writable by the host after load and before the first frame; `save_generation` increments on each write while $FFFC b3 (or the EEPROM window) is enabled; the host flushes when it changes; the core does no file I/O [ARCH.04 §4][ARCH.05 §1]. Not saved: pending host input, hooks, caller buffers.

## 8. Observability
- **Counters, always kept** (saved; reset by power cycle): per frame, `vdp_writes_active` (data/control-port writes during active lines), `vdp_fast_accesses` (data-port accesses < 26 T-states apart during active lines, `VDP_MIN_ACCESS_CYCLES`) [ARCH.02 4.A], `sprite_overflow_lines`, `collision_frames`, `lag_frames`, `frame_ints`, `line_ints`, `nmis`, `open_bus_reads`, `audio_dropped`; per line of the last frame: hscroll, display enable, sprite count, overflow and collision flags, data-port write count (≈ 6 B × 313). **Flip** to traced builds only if Cachegrind shows > 1 % [ARCH.06 §8].
- **Pure inspection.** `peek` reads the bus view without status-flag clears, read-buffer refills, address increments, open-bus updates, TH latching or lag credit. Property test: peeking all 64 KiB of BUS plus VRAM and CRAM leaves the state hash unchanged [ARCH.05 §5][ARCH.06 §3].
- **First fault, from phase 1** [ARCH.07 XFER-055]: the core records the first access since reset that hits an unmapped address or port, an illegal opcode class, a mapper write out of range, or a VDP data-port write while the display is active and faster than the 26-cycle rule, as {frame, line, dot, pc, opcode, addr, width, device, kind}; `get_fault` returns it and the runner prints `--trace first-fault`'s one line [ARCH.05 §4]. Recording costs one branch on already-cold paths.
- **Hooks (v5).** Instruction, bus-access, port, line and frame observers. `run_loop.inc` is compiled twice (`COGMASTER_TRACE` 0 and 1); the traced copy is chosen once per run call, only when hooks are installed. Proof: with no hooks, Ir/frame stays within 32 of a `COGMASTER_TRACE_LOOP=OFF` build [ARCH.06 §8, §10].

## 9. Boot
| State | Fast boot (default; facts only, no BIOS bytes) [ARCH.01 C7] | `--boot bios` (user's dump) |
|---|---|---|
| Z80 | PC $0000, SP $DFF0, AF $FFFF, other registers 0, I = R = 0, IM 0, IFF 0 | power-on reset state (PC 0, SP $FFFF), BIOS mapped per $3E bit 3 |
| $3E, RAM | $AB to the port and to $C000 (GG: $A8 to the port only); `ram_fill` first | whatever the BIOS leaves |
| $3F, mapper | $FF; $FFFC $00, $FFFD 0, $FFFE 1, $FFFF 2 | BIOS writes none (v1.3) |
| VDP, PSG, $F2 | regs zero except R2 $0E, R10 $FF; VRAM, CRAM zero; status clear; PSG silent; $F2 %00; GG $06 $FF | set by the BIOS |
| Beam | frame origin: line 192, tick 607, frame 0, ticks 0; a frame runs from frame-interrupt tick to frame-interrupt tick, so starting there makes every frame exactly one frame long | same |

- **BIOS** bytes are user-supplied only, recognised by the MAME digests in [ARCH.01 §6] (v1.3 `0072ed54`, Japanese v2.1 `48d44a13`, GG `0ebea9d4` …); any other 8 KiB / 1 KiB image is accepted with a warning; the BIOS checks the header, region nibble and checksum itself, so a rejected cartridge shows SOFTWARE ERROR as on hardware, never a core status. Lands v2.0; `load` with `boot = BIOS` and no BIOS falls back to fast boot with a warning flag [ARCH.05 §1].
- **Reset.** `reset(SOFT)` re-applies the hand-off table and keeps work RAM, cart RAM, VRAM and CRAM (no model has a CPU reset line a user can reach; the SMS1 Reset button is an input on $DD b4, not a reset); `reset(POWER)` also applies `ram_fill` and zeroes VRAM and CRAM. The adapter's Restart contract is `SOFT` on SMS and `POWER` on GG, separate from the SMS Reset input (context D-08) [ARCH.01 §7][ARCH.05 §1].
- **Refusals.** `UNSUPPORTED_BOARD` for a Korean, MSX, Janggun, 4PAK, Hicom or multicart detection before v2; `UNSUPPORTED_DEVICE` for an 8 KiB GG image (X-Terminator) or a Master Gear / Card Catcher entry; `UNSUPPORTED_SIZE` outside 1 KiB–1 MiB + 512; `PROFILE_REJECTED` for PAL + GG or PAL + JAPAN; an SG-1000 image (hash table or `.sg`) is `UNSUPPORTED_BOARD` with the message "SG-1000 mode is planned". The runner exits 77 for all of these [ARCH.04 §2.2, §6][ARCH.05 §4].
- **Test card** (phase 1, `testcard.c`): static; SMS 256 × 192 showing the 64 SMS colours as 15 × 15 swatches on a 16-px grid with 1-px backdrop borders; GG 160 × 144 inside the window showing all 4,096 RGB444 colours as 2 × 2 cells; both with orientation, crop-edge and channel-order markers, drawn in software (never through the VDP) into the normal field buffer and crop so the hash path is the real one; exact layout in phase 1 CONTEXT D-15 and `docs/`. No content → test card in runner and adapter [ARCH.05 §1].

## 10. Seams kept for later
| Later feature | Seam kept now | Lands |
|---|---|---|
| SG-1000 TMS modes | mode decode M1–M4 in `vdp.c` with a `switch`; the fixed TMS palette table and CRAM 16–31 rule reserved; 4-sprite slot constant; profile `machine = SG1000` enum value reserved [ARCH.02 §7] | SEED-006 |
| Light Phaser | `io_pins(port, tick)` may ask the VDP for the emitted RGB444 at any (line, dot) and request the H-counter latch at a TH edge; aim window and luma threshold per [ARCH.04 §3.3] | v2 |
| 3-D glasses | the $FFF8 latch is written by `mapper.c`'s RAM-mirror path and exposed in `cogmaster_info` per frame | v2 |
| Sports Pad, Paddle, MD pad | pin-level port model with TH/TR directions already in `io.c`; axes reserved in `cogmaster_input` | v2 |
| Gear-to-Gear link | GG $01–$05 decoded and saved; NMI source enum has a LINK value | v3 |
| Per-dot VDP | the line renderer takes (start dot, end dot); fetch-phase constants from [ARCH.02 §3.2] named but unused | v4 where measured |
| Korean, MSX, Janggun, 93C46 | `switch (board)` with reserved enum values; opcode-scan results already in `cart_info` | v2 |
| Movies, sub-frame input | `cogmaster_input` is size-tagged; a tick-stamped queue appends without an ABI break | v3 |
| Hooks, `cogmaster_run` | `run_loop.inc` dual compile from phase 2; stop-reason enum from phase 1 | v5 |
| Big-endian, 32-bit, WASM, consoles | byte-wise LE; `be32` and `wasm32` per PR from phase 1; `size_t` arithmetic checked on ILP32; no threads, files or globals [ARCH.06 §9, §14] | lanes P1 |

## 11. Build order for v1.0
| Phase | Tracer first (one path through every layer) | Then widen to | Done when |
|---|---|---|---|
| P1 A test frame in RetroArch | `testcard.c` → field buffer → crop → runner `--hash` → libretro → RetroArch screenshot | six platforms + `be32` + `wasm32`, asan/nofp/hygiene, state visitor round trip, first-fault record, attested release, install line | runner and libretro hashes equal on every leg; the screenshot equals `--png` |
| P2 The Z80 matches the vectors | `z80.c` against the committed SingleStepTests sample, then the full set nightly | bus decode and open bus, Sega mapper, Codemasters and none, $3E/$3F/$DC/$DD and GG $00–$06 in the `ports` fixture, Pause NMI at tick 611, fast boot, `--probe`, loader and run fuzz targets | `ctest -L vectors` green; a WLA-DX fixture boots to a RAM predicate |
| P3 Games play in RetroArch | one mode-4 line from a unit test → the SMS fixture via fast boot → the GG fixture in the window → the compatibility ledger | interrupts at 607/608, latches at 600, sprites and collision, both pads, regions, battery span, SMS1 and GG profiles, owner's dumps via the `library` preset | fixture frame hashes pinned on every leg; boot-to-play in RetroArch; ledger rows for the kickoff fixtures |
| P4 Sound | PSG unit vectors → transition hash → `synth.c` → one integrated RetroArch session | GG stereo, YM2413 port, $F2 per profile, PCM hashes, JoppyFurr ROMs built in CI, `--vgm`, clipping check | transition and PCM hashes equal on every leg; zero clipped samples, peak ≤ −1 dBFS [ARCH.03 §7] |

Within a phase, opcode groups and devices that share no inventory run as parallel plan waves (z80 ALU, z80 block/bit groups, `bus.c`, `mapper.c`, `io.c` in P2; renderer, sprites, interrupts and pads in P3); shared-inventory plans stay sequential [ARCH.07 XFER-066]. Each phase ends in a release and the release PR is never held [ARCH.09].

## 12. Portability and performance
- Integer-only; 32-bit `size_t` safe (state size, field size and ROM padding computed in `uint32_t` with overflow checks); big-endian safe through the byte-wise serialiser and the `be32` lane; wasm32 through the WASI lane; zero allocations after load; no recursion, VLAs or bit-fields [ARCH.06 §2, §9, §14].
- **Hot path** = the per-line renderer: 32 name-table words, 256 dots of priority select, ≤ 8 sprites of 8 or 16 dots, collision, then the sprite scan for the next line. Keep its state in one struct (`vdp_line`) of contiguous fixed-width fields; the field buffer is row-major `uint16_t[240][256]`; CRAM is stored pre-expanded to RGB444 so a dot is one load and one store. The Z80 dispatch is one `switch` per opcode page; the bus fast path is one compare against `next_deadline`.
| Lane | What it proves for the core | From |
|---|---|---|
| six native legs | identical `hashes.txt` (frame, transition, PCM, state) | P1 |
| `be32` (ppc32 under qemu) | endianness of the serialiser and `uint16_t` pixel writes; ILP32 `size_t`; unsigned `char` | P1 |
| `wasm32` (wasi-sdk + wasmtime) | no float helper imports, no libc surprises, 32-bit state-size math; the browser build's core | P1 |
| `asan`, `nofp` | UB, bounds, leaks; no FP instruction emitted | P1 |
| `armhf`, console compile-only | 32-bit ARM handhelds; devkitPro/pspdev toolchain files | nightly, later |

- **Budgets.** Ir/frame per PR (report-only until two zero-spread controls, armed by phase 2) [ARCH.06 §8][ARCH.07 XFER-051]; handheld ceilings `device_mips × frame_µs / 2` come from SMS-ECOSYSTEM-AND-USERS §4, not yet written [ARCH.11]: **pending**, the ceiling row stays empty in `quality.json` until that file lands [ARCH.06 §14]. Comparator only: the NES sibling runs at 23.3–24.7 M Ir/frame; a Z80 at 3.58 MHz with a per-line VDP should land near it (hypothesis, measured in P3).
- **Perf subject.** Through phase 2 the `perf` job and the nightly bench measure the no-content path (the test card). The phase 2 fixtures stop in a HALT loop once they have measured, so their instructions per frame describe no real workload; the first representative subject is phase 3's `boot_to_play`, and arming the Ir/frame gate stays the post-merge `ci:` pull request's job.

## 13. Open conflicts and how each is settled
| Conflict | Positions | Settlement | Local test that settles it |
|---|---|---|---|
| vscroll latch | per frame (Charles, wiki) vs per line (ares, MiSTer) [ARCH.02 §3.3] | **Rec** per frame: latched at the frame-INT tick; a mid-frame reg 9 write shows next frame. **Alt** per line at tick 600. **Flip** VDPTEST or a harness ROM shows a mid-frame reg 9 write moving the picture within the frame | `vdp.vscroll.latch_point`: a WLA-DX ROM writes reg 9 at line 100 and the frame hash is compared with both models; VDPTEST as a released binary |
| SMS1 zoom slot rule | wiki/sverx "slot i is wide only if slot i + 4 exists" vs MAME "first four" [ARCH.02 3.A] | **Rec** wiki rule, SMS1 profile only. **Alt** MAME. **Flip** a capture shows a lone zoomed sprite 16 px wide on an SMS1 | `vdp.sms1.zoom_slots`: two zoomed sprites 15 px apart must not collide (sverx's detector) |
| Interrupt tick positions | Charles 607/608 vs MiSTer x = 485–488 (≈ 970–976 ticks of its own origin) vs FluBBa ≈ 654 [ARCH.01 §9][ARCH.02 §4.2] | **Rec** 607 frame, 608 line, latches 600, V counter 604 (one constant set, named). **Alt** MiSTer's VDPTEST-tuned values. **Flip** FluBBa's IRQ tester reads HC ≠ $06/$07 at handler entry here | `vdp.int.hcount_at_entry`: handler's first `IN ($7F)` equals $06 or $07; `vdp.vcount.read_sequence` per mode |
| Open-bus byte | SMS1 last instruction byte (which byte is untested) vs $FF everywhere [ARCH.01 C5] | **Rec** one data-bus byte updated by every fetch, read and write, returned on SMS1/Mark III, $FF on SMS2/GG. **Alt** $FF everywhere. **Flip** never for SMS1; a vector that distinguishes operand from refresh byte refines it | `bus.open_bus.sms1_vs_sms2`: `IN A,($3E)` after a known instruction under both profiles |
| Access-slot corruption | hardware drops or mis-addresses fast data-port writes; no SMS slot schedule published [ARCH.02 §4.8] | **Rec** v1 never corrupts; count `vdp_fast_accesses` and record the first as a fault. **Alt** a per-line slot model. **Flip** a corpus title shows garbage that hardware also shows, or a schedule is published | the counter column in the ledger; sverx twin-ASIC test as a binary |
| FM/PSG mix ratio | MAME 1.0/1.0 vs listeners 0.25–0.35 [ARCH.03 §4.4] | **Rec** psg 1434 / fm 4096 Q12, behind the transition hash so a retune moves only PCM digests. **Alt** 1.0/1.0. **Flip** a line-out capture of the Japanese BIOS theme gives a ratio outside 0.25–0.5 | `audio.mix.no_clip` now; the capture at v4 |
| Overflow and collision outside the active area | MAME gates overflow by "active and no VINT", MiSTer by y range; collision only inside the active area with display on; VDPTEST's off-screen case passes through stale SAT state [ARCH.02 §3.5, §3.7, §10] | **Rec** active lines only, display on, column 0 under the mask excluded. **Alt** MiSTer's y window. **Flip** a title in the collision list (Ecco, Taz-Mania, Impossible Mission) misbehaves | `vdp.sprite.overflow_gate`, `vdp.sprite.collision_gate` fixtures; the collision titles in the ledger |
| GG port $06 in SMS mode | not decoded (Charles, MAME) vs decoded always (ares) [ARCH.03 GG-STEREO] | **Rec** not decoded: the write falls through to the $3E mirror, mask stays $FF, dual mono. **Alt** decode always. **Flip** headphone panning from a $06 write on a real GG in SMS mode | `psg.gg.port06_sms_mode`: mask unchanged after the write under `gg_mode = SMS` |

## Sources
| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| ARCH.01 | T1 | `.planning/preparation/SMS-SYSTEM.md` (this project's research) | 2026-10-18 | tick unit, line timeline, Z80 acceptance, maps, ports, profiles, fast boot, power-on, C1–C8 |
| ARCH.02 | T1 | `.planning/preparation/SMS-VDP.md` | 2026-10-18 | revisions, registers, pipeline, timing constants, geometry, palettes, choices 1.A–9.A |
| ARCH.03 | T1 | `.planning/preparation/SMS-AUDIO.md` | 2026-10-18 | PSG, GG stereo, YM2413, synthesis, rationals, hashes, output contract |
| ARCH.04 | T1 | `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` | 2026-10-18 | header, probe order, mappers, ports, pads, saves, hostile bounds, milestone placement |
| ARCH.05 | T1 | `.planning/preparation/HOSTS-AND-INTEGRATION.md` | 2026-10-18 | libretro, rcheevos, runner, Playstead, lag frame, host/core split |
| ARCH.06 | T1 | `.planning/preparation/ENGINEERING.md` | 2026-10-18 | C rules, tests, lanes, Ir gate, style, no-alloc check, WASM |
| ARCH.07 | T1 | `.planning/LESSONS.md` (XFER-005, 051, 055, 070, 071) | 2026-10-18 | CPU from vectors, early perf gate, first-fault capture, integer timeline and no host clock, switch dispatch |
| ARCH.08 | T1 | muhtari `.planning/preparation/ARCHITECTURE.md` §4, §7, §8 (owner's sibling; format exemplar and console-agnostic API, visitor and observability conventions, themselves from nesturbator `src/state.c`, `include/nesturbator.h` at `34d68516a15e`) | 2026-10-15 | size tags, statuses, allocator rule, version table, four-pass visitor, header layout, dual-compiled loop; no 7800 fact copied |
| ARCH.09 | T1 | `.planning/PRODUCT-ROADMAP.md`, `AGENTS.md`, `.planning/preparation/BRIEF.md` | 2026-10-18 | four v1.0 phases, deliverables, core rules |
| ARCH.10 | T1 | Arithmetic run here (Python `fractions`) | 2026-10-18 | samples per frame rationals, overshoot bound 607 + 69 = 676 < 684 (tested) |
| ARCH.11 | pending | `SMS-ECOSYSTEM-AND-USERS.md` §4, `CONFORMANCE.md`, `PROVENANCE.md` (not yet written) | none | handheld Ir budgets, fixture and vector names, seed notices; cited as pending in §3, §11, §12 |
