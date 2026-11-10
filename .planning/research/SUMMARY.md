# Project Research Summary

**Project:** cogmaster, milestone v2.0 "The whole library" (additive to the shipped v1.0 core, v0.4.0)
**Domain:** Deterministic, integer-only C17 Sega Master System / Game Gear emulator core: library, headless runner and libretro core
**Researched:** 2026-11-09
**Confidence:** MEDIUM overall (HIGH on scope, stack and list counts; MEDIUM on peripheral and BIOS hardware behaviour; LOW on other emulators' `.srm` bytes)

## Executive Summary

v2.0 adds no runtime dependency, vendored file or build tool. Each capability either extends a mechanism that shipped in v1.0 (the SHA-1 hash table, the battery span with its generation counter, the atomic runner save, the VDP H-counter latch, the YM2413 unit) or is roughly 100 to 400 lines of own C written from a public datasheet or from a BSD-3, ISC or CC0 file the project may read. A core like this grows one board or device at a time, behind invariants it already enforces: ticks are VDP clocks, devices catch up lazily to the access tick, all state lives in the instance, and every call is bounded. The recommended shape follows that pattern. Leave the proven Sega and Codemasters paths alone, put each new mapper board in a new `switch` arm in a new file, add a pin-level port layer before any peripheral, and ship the Light Phaser alone as the last controller, because it is the only item that depends on VDP latch timing.

The scope is fixed by the CC0 MAME software lists at the existing pin `1d7a454ed79e`: 33 SMS entries on non-Sega boards, 7 Game Gear EEPROM entries across four base games, 13 list-flagged Light Phaser dumps, 4 paddle, 3 sports pad and 9 3-D glasses titles, and a compatibility ledger seeded from the hash table (1,657 rows in `src/cartdb.inc`). Two limits are engineering constraints rather than libraries. The hash table is 557,877 bytes and will grow, so its row layout is compacted in the mapper phase. The 40,960-byte cap on each `docs/*.md` file cannot hold a per-row compatibility page, so that page becomes aggregates. Several test and docs budgets are at or near their caps, so the tune-up phase has to make room first.

The risks are correctness failures that a green build will not reveal on its own: a heuristic mapper scan that turns Sega games into Korean boards; an EEPROM built as the Sega RAM window, which corrupts saves silently; `.srm` interop that is assumed rather than measured; a Light Phaser that polls instead of latching at beam time; and a BIOS path that lands before its firmware legality guards exist. Each has a prevention rule and a phase in Critical Pitfalls. Three scope questions (Mark III, SG-1000 rows, and the meaning of the `is_experimental` exit) need the owner's answer before requirements are final. Defaults are given under Gaps.

## Key Findings

### Recommended Stack

Zero new runtime dependencies, zero vendored files, no new language or build tool. Repository facts checked on 2026-11-09: the hash database, its generator (`tools/cartdb.c`) and its binary search already exist; the runner's atomic save and the battery span with generation counter already exist; SHA-1, SHA-256, CRC-32 and MD5 are in tree; the MAME pin is still current for these lists (last SMS and GG change `954def46685c`, 2026-09-04, before the pin of 2026-10-18), so no pin bump.

**Core technologies:**
- **Extended `tools/cartdb.c` generator:** maps MAME `slot` values to boards and sets EEPROM persistence from `slot == "eeprom"`. The 7 GG EEPROM entries carry no `battery` feature, so a battery-flag default would mark them volatile and the save would silently do nothing.
- **Compacted hash row:** 20-byte binary SHA-1, small fixed-width facts, and titles moved to a ledger-side file. Estimated at about 66 KB from 557 KB for the same rows. Keep binary search (about 11 comparisons); no perfect-hash tool.
- **Own `src/eeprom.c`:** 93C46 Microwire state machine, 64 little-endian 16-bit words, 128-byte image held in `cart.ram`. MAME's BSD-3 `eepromser.cpp` is a cross-check oracle, not a seed.
- **Own Korean signature scan in `src/cart.c`** (about 80 lines), replacing the raw-byte `other_board()` (see Where the Research Disagrees, item 4).
- **libretro `RETRO_DEVICE_LIGHTGUN` and `RETRO_DEVICE_MOUSE`:** already in the vendored `libretro.h` 1.22.2.
- **BIOS recognition by SHA-1** in a small sorted table; digests are already recorded in `ASSET_POLICY.md` section 4. No BIOS byte is committed or embedded.
- **POSIX sh, `gh`, `jq` and CMake `string(JSON)`** for the tune-up (timeout derivation, weekly digest, `quality.json` ratchet). No Python and no coverage lane.

**Do not add:** XML, JSON, SQLite, compression or CSV libraries; a Python or Node step in build or CI; any code, copied or read, from Genesis Plus GX, Gearsystem, MEKA or BlastEm (GPL or non-commercial, never opened). A web search for 93C46 wiring returns a GPGX `eeprom_93c.c` page; do not open it. Released binaries and their documentation may be used.

### Expected Features

**Table stakes (milestone scope):**
- **Korean boards** detected with no setting: `zemina` (8), `korean` at $A000 (6), `korean_nb` (5), `hicom` (9), `nemesis`, `4pak`, `janggun`. Hash first, opcode scan as fallback. FEATURES reports about 1,057 SMS dumps in No-Intro against 653 CC0 entries, so the scan is needed for dumps the list does not hold.
- **93C46 EEPROM** for the four GG baseball base games (seven entries: The Majors Pro Baseball; World Series Baseball v0 and v1; WSB '95; Pro Yakyuu GG League and '94; Nomo's WSB), saved as a 128-byte `.srm`, written atomically by the runner.
- **Sports Pad** (3 titles), **Paddle** (4 required titles, Japan-region profiles only) and **3-D glasses** eye state (9 titles; 13 list entries carry the note).
- **Light Phaser** at pin level for the 13 list-flagged dumps, plus two unflagged titles (Laser Ghost; Hang-On and Safari Hunt).
- **Default controller device** from the hash table, with the user option winning. Per-profile device availability is enforced in the profile, not per device.
- **BIOS boot** from the user's own dump, optional, default fast boot. Baselines are the 8 KB SMS and 1 KB GG BIOS.
- **PAL and Japanese FM rows** under the correct profile, with an audio predicate for FM titles.
- **Ledger over the CC0 rows** and removal of `is_experimental` on a stated, tested rule.

**Differentiators (cheap once the base exists):** Light Phaser sensed from the emitted pixel at beam position, with no per-title offset table; gun, paddle and sports pad input recorded per frame, so replays and CI hashes cover peripheral games; anaglyph and left/right composition in the host, outside the frame hash; an adapter-drawn crosshair; `--probe` reporting board, persistence, controller default and the evidence behind each (hash hit, scan counts, board source); a fast-boot versus real-BIOS equivalence check on the owner's dump; a hosted save round trip for EEPROM and battery; an `audio@F=H` predicate over the transition stream.

**Defer:** built-in-game BIOSes (SMS2 128 KB and 256 KB; v2.x, only if the owner's dump is one); Mega Drive pad (v2.x, cheap on pin-level ports); any `.srm` pad-on-import rule (only after measurement); Seo Jin and 188-in-1 (one title each; 188-in-1 is `supported="no"` in MAME and its entries 6 and 13 fail in MAME and ares, so they get ledger rows that name the failures, never a hack); Gear-to-Gear cable, SK-1100 keyboard, Graphic Board, SG-1000 and SC-3000 boards, cheat devices (X-Terminator, Pro Action Replay, Master Gear), all named `unsupported` with exit 77.

**Anti-features (do not build):** per-game Light Phaser offset tables; guessing the layout of a foreign save file; a fixed 32 KB `.srm` for every game; a bundled or required BIOS; BIOS region lock on by default; FM forced on for every game; crosshair or anaglyph baked into the hashed frame; `pass` seeded from the core's own output; uploading the owner's frames, saves or file names.

### Architecture Approach

The v1.0 invariants hold: ticks are VDP clocks; the Z80 drives `bus.c`; devices catch up lazily; the VDP renders monotonically; one instance holds all state; no function-pointer tables (`abi.globals`); zero allocation after load (BIOS and EEPROM memory are taken at load); every call bounded. ARCHITECTURE section 1 found six shipped facts that change scope, and the roadmap must plan from them: the public API is at 9 with no public state calls (libretro reports `savestate = "false"`), so state-format bumps are free until v3.0; the atomic runner save already ships; `cogmaster_set_input` copies `sizeof` the current struct after checking only `size >= 32`, which becomes an over-read the moment the input struct grows; `io.c` has no device abstraction; the H-counter latch reads only the current tick; and `boot.c` performs only the fast hand-off.

**Components:**
1. `src/mapper_ext.c` (new): read and write arms for non-Sega boards, one `switch` arm per board. The Sega, Codemasters and ROM-only arms stay unchanged.
2. `src/eeprom.c` (new): 93C46 state machine. The 128-byte image lives in `cart.ram`, so the battery API, runner and libretro need no special case.
3. `src/ports.c` (new): pin levels per port per device at a tick (pad, paddle, sports pad). Mixing with `$3F` stays in `io.c` with its existing tests.
4. `src/lphaser.c` (new): lazy sensor sampler reading `field[][]` after `render_until`.
5. `boot.c` and `instance.c` (modified): `boot_power_on()` beside `boot_handoff()`; BIOS identification; BIOS media copied at load.
6. `bus.c` (modified): one precomputed `slow` flag replaces the per-read `$3E` mask test; BIOS window; `$FFF8-$FFFB` glasses snoop.
7. `vdp.c` (modified): `vdp_hcount_at(t)` and `vdp_latch_h_at(t)`; the current latch becomes the `t = bus.t` case.
8. `state.c` (modified): new fields with bounds and cross-checks; the `_Static_assert` tripwires are updated in the same change.
9. Host (runner and libretro): `--boot`, `--bios`, `--port1`/`--port2`, `--save-interval`; firmware lookup through the system directory; controller subclasses; crosshair and eye composition in a host helper outside the hash.

**Patterns to follow:** switch arms rather than tables; one `slow` flag on the bus read path; every beam-dependent device observed at three points (before an access that can see it, at a dispatcher event before a repaint, and at frame end); append-only structs with a zero-defaulting copy; every state field bounded and cross-checked against `ticks`.

**The one TH edge detector:** every TH change (a `$3F` write, a gun transition, a sports pad output) goes through `th_changed(old, new, tick)` in `io.c`. The shipped rule (latch on a low-to-high effective TH edge) moves into that function unchanged, so existing hashes cannot move.

### Critical Pitfalls

1. **Heuristic board detection (Phase 2).** The current `other_board()` counts raw `0x32` bytes over the first 32 KiB with no minimum and no instruction boundary. Once Korean boards exist, data tables containing `32 xx xx` would flip Sega games. Prevention: a valid `TMR SEGA` header with a passing checksum outranks the scan; the heuristic needs a threshold and a margin over the competing family, tuned on decoy images built from owned bytes; a tie refuses with the candidate named; the source of the board (`db`, `header` or `heuristic`) is reported by `--probe`.

2. **Sega bank arithmetic reused for 8 KiB boards (Phase 2).** Zemina, Nemesis, Janggun and 188-in-1 select 8 KiB banks; Hicom selects 32 KiB; the Sega code uses 16 KiB pages. Some boards also write to addresses the Sega board treats as faults. Prevention: every index reduced modulo the image length in `uint32_t`; an exhaustive per-board test that writes every value to every register and reads every address, under ASan; a per-board list of which writes are faults, and a test that documented register writes record no fault.

3. **EEPROM built as Sega RAM (Phase 3).** On the EEPROM board, `$FFFC` bit 3 opens a window at the single address `$8000`; every other slot-2 address still reads ROM bank 2 (MAME BSD-3, `sega8/rom.cpp`). `$FFFC` bit 7 resets the chip, where on the plain Sega board bit 7 is ROM write enable and the current code raises a fault for it. Prevention: a separate board kind; a state machine with write-disabled power-up, EWEN and EWDS; the generation counter bumped only when a completed WRITE, ERASE, WRAL or ERAL changes data, not on every clock edge; busy time counted in ticks if modelled at all; shift and busy state in the save-state visitor with a mid-command round trip; a bit-banger fuzz target.

4. **Light Phaser as a read-time poll (Phase 5).** Evaluating the sensor only when the Z80 reads `$DD` loses pulses between reads and latches H at read time instead of at the lit dot, so games aim a few pixels off or not at all. Prevention: the sensor evaluated incrementally on each VDP catch-up; H latched at the edge tick through `vdp_hcount_at`; zero behaviour change with no gun connected (a plugged and an unplugged run give the same frame hash); a bounded number of samples per line, whatever the poll rate.

5. **BIOS before its guards (Phase 6).** ASSET_POLICY forbids any BIOS byte, frame or state in the repository or a release. The two scans it names (`release.firmware_scan`, `local.firmware_window_scan`) are planned but do not exist in `tests/`, `scripts/` or `cmake/`. A BIOS-running state also holds a copy of BIOS code in work RAM. Prevention: the guards, with negative controls, land before any BIOS code; CI runs a self-authored WLA-DX image shaped like an 8 KiB BIOS; real-dump runs stay local.

**Other pitfalls, with the phase that owns each:**

| # | Pitfall | Phase | Prevention |
|---|---|---|---|
| 6 | Budgets at cap: `docs/testing.md` 40,953 of 40,960 bytes; RetroArch test code 299 of 300 lines; check code 528 of 744 lines | 1, all | New prose goes in topic files (`docs/saves.md`, `docs/peripherals.md`, `docs/boards.md`); reclaim lines in the change that needs them; never raise a cap |
| 7 | Gates and timeouts set against pre-milestone code: 14 CI jobs still `provisional`; the `boot_to_play` allow line expires 2027-01-07 | 1, all | Re-measure timeouts for each lane a phase slows, on its first hosted run; re-baseline the perf allow line before arming the gate |
| 8 | `.srm` guessed: another core may write a larger file for an 8 KB game; a wrong word order keeps the length right and scrambles the contents | 3 | Measure released binaries once; pad on import, never on export; claim only what is tested |
| 9 | Save durability: flush happens only at exit and on SIGTERM | 3 | Frame-count flush (`--save-interval`); bounded sweep of stale `.tmp-N` files; directory fsync after rename on POSIX (verify absence first); kill tests |
| 10 | Paddle and sports pad: microsecond constants and host or float timing break determinism; a free-running paddle toggle can alias a game's 256-sample detection loop to 0% or 100% ones | 4 | Tick constants only, derived in comments (1 tick is about 93 ns; 1 Z80 cycle is 3 ticks); detection ratio held at 30 to 70% across several loop lengths; timeouts evaluated lazily at the next port access |
| 11 | Device state outside the visitor; ABI renumbering; ledger key that ignores the device; `cogmaster_set_input` over-read | 4 | Device ids appended after NONE; input fields appended; state validated; `cfg:` key carries the device; copy `min(tag, sizeof)` and zero the rest in the same change as the first axis field |
| 12 | 3-D glasses: `$FFF8-$FFFB` writes seen while the cartridge is disabled, and the wrong hook placement; a mid-frame shutter change (about 10 lines) is not modelled | 4 | Gate on `$3E` cart enable and an attached flag; inert on SMS2 and GG; eye reported at frame end and the limit documented; glasses writes never count as lag polls |
| 13 | PAL: the CC0 lists have no TV field; a text rule ("Europe") mislabels Brazil (PAL-M, which TV detection counts as NTSC) | 7 | No text rule; split ledger `tv` from `region` before any PAL row; PAL judged by owned carts and fixtures in all eight lanes (V-counter tables, Pause at line 311, exact samples per frame) |
| 14 | FM rows without an audio predicate, or without `fm` in the row key; a golden PCM hash freezes a provisional mix | 7 | `audio@F=H` over the transition stream with a negative control; `fm` in the row key; PCM only as a secondary check |
| 15 | Ledger claim inflation: `boots` is a smoke predicate; seeding `unsupported` breaks the ratchet; a per-row page breaks the docs cap | 8 (seeding rules in 2) | Seed `unknown` or a named terminal status; ratchet change with a negative control; aggregates in `docs/compatibility.md`; full list in `tests/compat/ledger.csv` |
| 16 | Table growth and generator drift: rows are 336 bytes; an unmapped `slot` silently becomes board 0; a pin bump reclassifies rows unseen | 1, 2 | Compact rows; generator fails on an unmapped slot and prints counts per slot; `size.*` ceilings in `quality.json`; pin bumps come with a classification diff |
| 17 | Hostile input: each new state machine and mapper arm parses ROM-controlled bytes | 2 to 5 | Pure functions of `(state, tick, input)`; bounded work per line or per call; each device added to the fuzz target in the same plan |
| 18 | Fixture sprawl and shared misreading: one author writes both fixture and implementation, so a pass proves only the author's belief | 2 to 4 | One parameterised source per family; ambiguous registers carry both candidate behaviours and a cited source; both-fail rows are `fail` or `skipped` |

## Implications for Roadmap

Eight phases, in dependency order. Each ends in a release (project rule 10). The PITFALLS labels P1 to P7 map as P1 to 1, P2 to 2, P3 to 3, P4 to 4 and 5, P5 to 6, P6 to 7, and P7 to 8. The roadmapper may renumber.

### Phase 1: Tune-up and budget headroom
**Rationale:** The perf gate and timeouts must be armed before the bus and I/O hot paths gain branches, and measured against a baseline. Docs and test budgets need headroom before any feature plan touches them.
**Delivers:** timeouts re-derived from the last 40 successful runs (no `provisional` rows on lanes later phases slow); the perf gate armed after two zero-spread control runs, with `boot_to_play` re-baselined on current `main` (delete the allow line if the new baseline passes, otherwise rewrite it with a current reason); `quality.json` with `size.zip` and `size.wasm` ceilings and a `string(JSON)` checker; the docs split into topic files; RetroArch test lines reclaimed; a README ROM step and a one-sentence BIOS legality line; review rows WR-02 and WR-04 closed or seeded; the weekly digest.
**Addresses:** no player-visible feature; enables all others.
**Avoids:** 6, 7, 16 (ceilings).
**Research flag:** standard; follows existing repository patterns (`tests/cmake/*.cmake`, `ENGINEERING.md` section 8).

### Phase 2: Mappers (Korean family, hash-first detection)
**Rationale:** Lowest risk and highest ledger yield (33 SMS entries). It also changes the hash row layout that every later phase reads (`slot`, persistence, controller defaults), so compaction and the generator change belong here.
**Delivers:** `korean_nb` mapped to ROM-only in the generator (no new code); `src/mapper_ext.c` with Korean $A000, Hicom, Zemina and Nemesis (29 of the 33 entries), then Janggun and 4PAK if cheap; 188-in-1 and Seo Jin last, or as `unsupported` rows that name what fails; `guess_board()` with threshold, margin and tie refusal; `--probe` reports `board_source`; the compacted row layout; decoy images and one parameterised WLA-DX register fixture per family, using the existing mailbox layout.
**Implements:** the grown `cogmaster__mapper` (extra 8 KiB bank registers, Janggun flag) with per-board index bounds. Seo Jin's $FFF0 register lies outside the current `addr >= $FFFC` gate; defer it or add a board-checked early return.
**Avoids:** 1, 2, 16, 17, 18.
**API and state:** API 9 to 10 (board constants); state format 11 to 12. No existing frame or audio hash may move; the eight-lane equality job is the proof.
**Research flag:** needs research on the Janggun reverse-bit flag (bit 6 per SMS Power; `>= $80` with a 6-bit mask per MAME) and on scan thresholds, tuned on decoys.

### Phase 3: Saves (93C46 EEPROM, `.srm` interop, durability)
**Rationale:** EEPROM is a Sega variant and rides on Phase 2's split. The `.srm` byte measurement is a planned task here and must finish before any import rule is written. Durability belongs with the path it protects.
**Delivers:** `src/eeprom.c`; an EEPROM board kind chosen from `slot == "eeprom"`; the 128-byte span through the existing battery API, with generation bumped only on completed changes; state fields with a mid-command round trip; the `.srm` byte order and sizes measured once from released libretro binaries (command, exit status and sizes recorded in the phase summary, nothing committed); `libretro_saves = "true"` with its guard test; `--save-interval` flushing by completed frames; the stale-temp sweep and directory fsync; a hosted save round trip inside the RetroArch budget after reclaiming lines; an EEPROM target in the fuzz suite.
**Addresses:** EEPROM titles (seven entries); `.srm` for battery and EEPROM titles.
**Avoids:** 3, 8, 9, 17.
**API and state:** state format 12 to 13; API unchanged, or +1 if a board constant is added.
**Research flag:** needs research: the 93C46 instruction tables (only a summary read; the full datasheet is not yet read); DO polling in the seven entries, which decides whether busy time is modelled (the default is immediate ready); the `.srm` measurement, planned as a task.

### Phase 4: Pad devices and 3-D glasses
**Rationale:** The port layer and the input-record change are shared prerequisites for the Light Phaser. This phase ships value alone (Sports Pad, Paddle and 3-D titles) and touches the riskiest ABI edge, the input struct.
**Delivers:** `src/ports.c`; `th_changed()` as the only latch path; a Sports Pad phase machine with tick timeouts (60 µs is about 644 ticks, 80 µs about 859, 143 µs about 1,536); a Paddle tick divider; the `$FFF8-$FFFB` snoop gated by `$3E` cart enable and an attached flag; `run_result.glasses` appended; `cogmaster_input` gains axes with the `min(tag, sizeof)` copy; device constants appended after NONE; `--port1`/`--port2` names; libretro controller subclasses; hash-driven default device; the ledger key carries the device. Pad-only frame hashes and the existing `io.*` tests stay unchanged.
**Avoids:** 10, 11, 12.
**API and state:** API to 11 or 12; state format 13 to 14.
**Research flag:** needs research on the Sports Pad timeout (143 µs in MAME against 60 µs in ares; both titles must track the ball, judged locally); the Paddle divider (see Where the Research Disagrees, item 10); and the input record model (item 9).

### Phase 5: Light Phaser
**Rationale:** The only item that depends on VDP latch timing and beam position. Highest risk; SEED-001 places it last. It ships alone so a regression in the latch path is isolated.
**Delivers:** `vdp_hcount_at()` and `vdp_latch_h_at()`; a lazy sensor sampler in `src/lphaser.c` over a radius-6 window, bounded per line; the latch edge rule settled by fixture and local runs; a named integer luma threshold with a test; the host maps screen coordinates through the crop (SMS 192, 224 or 240 rows; GG crop origin) to field coordinates; the adapter's crosshair; a self-authored fixture that lights a known square and polls TH at several loop lengths; the existing latch positions (600 and 604) unchanged; the plugged-versus-unplugged frame-hash equality test; the sticky "seen" flag cleared on a `$DD` read or at frame start.
**Avoids:** 4; the per-title offset anti-feature.
**API and state:** state format bump in this phase, if the sensor state is new.
**Research flag:** needs research on the TH edge rule (shipped rising edge; SMS Power latches while TH is low; MAME latches on 0 to 1; see item 12), the sensor shape (grid or first-lit-dot; see item 13), and which rows the radius covers. Released MAME and ares binaries serve as oracles; their source is not opened.

### Phase 6: BIOS boot
**Rationale:** Independent of Phases 4 and 5 except for API numbers and `bus.c` conflicts. Placed after them so the `$3E` power-on state and the `slow` flag are already proven.
**Delivers:** the two legality scans with negative controls, before any BIOS code; `boot_power_on()`; the BIOS window in `bus.c` behind the `slow` flag; the SHA-1 digest table; `--boot bios --bios FILE` (exit 77 when required and absent); firmware rows in the `.info` file; the adapter reads the system directory itself, with option `cogmaster_boot` defaulting to fast; the state header binds the BIOS identity; CI runs a synthetic BIOS-shaped image with no skip; an equivalence check against the fast-boot hand-off. The README says in one sentence that export BIOSes check the cartridge header, so some carts lock up under `--boot bios`.
**Avoids:** 5.
**API and state:** media fields and a status appended; state format 14 to 15.
**Research flag:** needs research on `$3E` arbitration (SMS-SYSTEM section 3 says outputs are ANDed when both are enabled; see item 6); the BIOS power-on register state; and the libretro firmware key format (MEDIUM; confirm against the current core-info repository).

### Phase 7: PAL and Japanese FM profiles
**Rationale:** Uses shipped TV and FM infrastructure (the 313-line PAL profile, YM2413). It needs the ledger's `tv` and `region` split before rows are written, and an audio predicate before FM rows.
**Delivers:** the ledger migration that splits `tv` from `region`, with a format note; PAL fixtures (V-counter tables, Pause at line 311, exact audio samples per frame, a TV-detection loop counted both ways) in all eight lanes; the `audio@F=H` predicate in the shared parser, with a negative control; `fm` in the row identity; PAL and FM rows judged by owned carts locally; the Mark III and FM profile only if the owner confirms it (see Gaps).
**Avoids:** 13, 14.
**API and state:** none expected.
**Research flag:** standard for fixtures; the PAL and FM judgements need owner-held carts run locally.

### Phase 8: Whole-list ledger and `is_experimental` removal
**Rationale:** Statuses depend on every board and device being built or named `unsupported`. It goes last because it is the phase most able to overclaim.
**Delivers:** ledger rows for the SMS and GG entries in scope (denominator per Gaps); `unsupported` rows that name the missing board or device and exit 77; `skipped` with a reason for MAME `supported="no"`; a ratchet rule for `unsupported` to `pass` with a negative control, only if rows must start that way; `docs/compatibility.md` regenerated as aggregates plus evaluated rows; `is_experimental` removed when the exit command passes.
**Avoids:** 15.
**Research flag:** the exit condition is a requirements decision (Gaps); its wording is needed before this phase is planned.

### Phase Ordering Rationale

- **Tune-up first:** gates and timeouts need a measured baseline before any branch is added to the bus and I/O paths.
- **Mappers before saves:** EEPROM is a Sega variant, and the mapper phase introduces the `slot`, persistence and controller-default columns every later phase reads.
- **Saves before peripherals:** independent of the port work, and the highest user-visible correctness risk, so it is taken while the mapper context is fresh.
- **Ports before the Light Phaser:** the gun needs `ports.c`, `th_changed` and `vdp_hcount_at`.
- **Light Phaser alone:** the highest risk, isolated so regressions in the latch path are attributable.
- **BIOS after peripherals:** the `$3E` power-on state and the `slow` flag are proven, and the guards precede the code. BIOS does not depend on the Light Phaser, so it could move ahead of Phase 5 if the Light Phaser is blocked.
- **PAL and FM before the whole ledger:** statuses depend on every profile existing, and the ratchet change comes last.

Phases 4 and 5 are the natural split if the roadmap needs smaller increments. Do not merge Phase 5 into Phase 4.

### Research Flags

**Needs deeper research during planning:**
- Phase 2: the Janggun reverse bit; scan thresholds and margin.
- Phase 3: the 93C46 datasheet read; DO polling in the seven entries; the `.srm` measurement; the directory-fsync claim in `runner/save.c` (see item 8).
- Phase 4: the Sports Pad timeout; the Paddle divider; the input record model.
- Phase 5: the TH edge rule; the sensor shape; the crop mapping.
- Phase 6: `$3E` arbitration; the BIOS identity scheme; the firmware keys.
- Phase 8: the wording of the exit condition.

**Standard patterns (planning can proceed without new research):**
- Phase 1: the tune-up follows existing repository patterns.
- Phase 7: PAL and FM fixtures follow shipped infrastructure.
- Phase 2 implementation, once the detection design is settled: switch arms in a new file.

## Where the Research Disagrees

The four research files conflict in the places below. The position taken here is the default; the phase that owns each item confirms it from its own evidence.

| # | Topic | Conflict | Position taken |
|---|---|---|---|
| 1 | API and state numbers | FEATURES: API 8, state 10. ARCHITECTURE, read from `include/cogmaster.h` at `b9c6eb7`: API 9, behaviour 11, state 11. | Use ARCHITECTURE. The FEATURES numbers predate the current tree. |
| 2 | Ledger denominator | FEATURES: 1,476 (SMS and GG, raw). ARCHITECTURE: 1,657 (`cartdb.inc`). PITFALLS: 1,690 (all three lists, raw). | 1,690 less 33 compound or incomplete rows gives 1,657. The 33 is inferred from the generator's exclusions, so confirm it from generator output. Do not quote any raw list count as the table count. Scope is in Gaps. |
| 3 | Mark III | FEATURES adds Mark III and FM profiles in v2.0. ARCHITECTURE puts Mark III out of scope pending the owner. | Out of scope until the owner decides. |
| 4 | Korean detection | STACK calls it "an own bounded signature scan". PITFALLS shows raw byte counting is the failure mode. | `guess_board()` matches at decoded instruction boundaries, needs a threshold and margin, and refuses on tie. `other_board()` is retired, not extended. |
| 5 | BIOS identity | STACK and FEATURES: SHA-1 digest. ARCHITECTURE: CRC-32 for identity and the state header. | SHA-1 for recognition (the same key as the cart table, with the collision resistance it needs). The state header binds a 32-bit value (CRC-32 or the first four bytes of the SHA-1), chosen in Phase 6. Recognition and state binding are separate needs. |
| 6 | BIOS and cartridge arbitration | ARCHITECTURE: BIOS wins at its window while `$3E` bit 3 is clear (exclusive overlay). PITFALLS: an exclusive overlay is subtly wrong, since SMS2 ANDs both outputs. | Do not implement an exclusive overlay until SMS-SYSTEM section 3 has been checked against the code, in Phase 6. |
| 7 | EEPROM window | ARCHITECTURE: slot-2 reads and writes are routed to `eeprom.c`. PITFALLS (from MAME BSD-3): only `$8000` opens the window; other slot-2 reads stay ROM. | The single address `$8000`. PITFALLS is more precise. |
| 8 | Runner save | STACK: keep `runner/save.c` as it is. PITFALLS and ARCHITECTURE: add a periodic flush, directory fsync and a stale-temp sweep. | Keep the atomic write path and add the three hardening items in Phase 3. Read `runner/save.c` before planning Phase 3 to settle whether directory fsync is present. |
| 9 | Paddle and sports pad record | ARCHITECTURE: the paddle is absolute. PITFALLS: motion is relative and the record carries deltas. | The host sends deltas and the core accumulates the position in state. Replays then match, and motion with no reader does not drift. |
| 10 | Paddle timing constant | ARCHITECTURE: about 596 ticks per cycle. PITFALLS: "half period about 597 ticks". At 18 kHz (a 55.6 µs period) the full period is about 597 ticks at 10.738 MHz, and the half period is about 298. | Define the constant as either the period or the half period and cite the derivation. The two files use the same number under different names. |
| 11 | Light Phaser per-title values | ARCHITECTURE allows a facts table keyed by SHA-1. FEATURES forbids a code table. PITFALLS allows a per-title value only as a generated-database fact, after a global constant fails. | One global delay constant first. Any per-title value goes in the generated database with ledger evidence, never a hand-written table in `src/`. |
| 12 | Light Phaser latch edge | The shipped rule latches on a rising effective TH (ARCHITECTURE). FEATURES: TH is low while the sensor sees light. SMS Power and MAME differ. | Keep the shipped rule as the default in `th_changed`. Settle it in Phase 5 by fixture, and by running both rules locally against the owner's Color and Switch Test and Hang-On cases. |
| 13 | Light Phaser sensor shape | ARCHITECTURE: a grid sampler, about 50 samples per frame. PITFALLS: scan the dots since the last evaluation for the first lit dot. | Both are bounded. Choose in Phase 5 by fixture. The cost cap is the requirement. |
| 14 | PAL assignment | FEATURES: PAL rows from "Europe" and "Australia" text. PITFALLS: no text rule; Brazil titles make text unreliable; there is no TV field. | No text rule. `tv` is a ledger configuration judged with owned carts. |
| 15 | `.srm` name for same-stem ROMs | STACK: one `.srm` per title, no second file. PITFALLS: add a platform tag or check machine and size. | Keep `<stem>.srm`, document the same-stem collision, and check machine and size on import. |
| 16 | `boot_to_play` allow line | STACK: re-baseline, delete the line, then arm. PITFALLS: keep it and re-baseline. | Re-baseline first. Delete the line if the new baseline passes; otherwise rewrite it with a current reason. |
| 17 | EEPROM entry count | "Seven titles" (STACK, PITFALLS) against "7 entries, 4 base games" (FEATURES). | Seven entries across four base games. |
| 18 | Table row count | 1,657 (STACK and ARCHITECTURE, measured) against 1,656 (PITFALLS). | 1,657, until the generator says otherwise. |
| 19 | Compatibility page size | About 300 bytes per row (STACK) against about 460 (PITFALLS). | Either way the full page exceeds the 40,960-byte cap by an order of magnitude. The fix is aggregates. |
| 20 | 3-D glasses titles | FEATURES: 9 titles use the shutter. PITFALLS: Blade Eagle, Maze Hunter and Missile Defense need the glasses; the rest have a 2-D mode. | Both are true. The default host output must be playable with one eye or in 2-D. |
| 21 | RetroArch test budget | ARCHITECTURE: the 300-line budget has one slot free. PITFALLS, measured: 299 of 300 lines used. | Use the measured figure. Reclaim lines in Phase 3, before adding the round trip. |

## Confidence Assessment

| Area | Confidence | Notes |
|---|---|---|
| Stack | HIGH | No new dependencies; table size measured; compaction estimate MEDIUM-HIGH; the 93C46 opcode list is MEDIUM until the full datasheet is read. |
| Features | MEDIUM | List counts HIGH (parsed from the pinned CC0 lists); peripheral behaviour MEDIUM (preparation files plus MAME BSD-3); `.srm` sizes LOW. |
| Architecture | HIGH for integration points | Read from the shipped source at `b9c6eb7`. Four hardware semantics are MEDIUM: TH edge, `$3E` arbitration, sensor shape and paddle clock. |
| Pitfalls | HIGH for repository-grounded items | Budgets and timeouts measured in the tree. Hardware behaviour is MEDIUM and is settled by each phase's own fixtures. |

**Overall confidence: MEDIUM.** The scope and the stack are sound. The uncertainty sits in hardware details that each phase settles with its own fixtures or owned-cart runs.

### Gaps to Address

**Owner decisions (needed before requirements are final; defaults given):**
- **Mark III in v2.0?** FEATURES adds it; ARCHITECTURE defers it. Default: out, pending the owner.
- **SG-1000 rows in the denominator?** The lists carry 214 SG-1000 entries, and SG-1000 is a dormant seed (SEED-006). Default: SMS and GG only, with the excluded count printed by the generator.
- **What does the `is_experimental` exit mean?** FEATURES: rows stay `unknown` until the owner's local run judges them. PITFALLS: no row may remain `unknown`. Both cannot hold for entries the owner does not hold. Default: the exit is a command over the rows the owner's local library judged; every `unsupported` row names its reason; unowned rows stay `unknown` with the count printed. Rule 8 bars a count that only a published release can show.
- **Real-hardware evidence:** none is planned. Light Phaser behaviour is judged against owned carts run locally and released MAME and ares binaries. If that cannot settle the edge rule, the owner's own console is the only evidence left, and project rule 6 allows asking for it only with a stated reason.

**Evidence gaps, each closed in its phase:**
- `.srm` size and EEPROM word order used by other cores (LOW): one measured run of released libretro binaries in Phase 3; record command, exit status and sizes.
- Janggun reverse bit, bit 6 or bit 7 (LOW; the sources disagree): an owned-cart trace, or a fixture that exposes both candidates.
- Sports Pad timeout, 143 µs or 60 µs (LOW): both titles must track the ball; run locally.
- 93C46 write-busy behaviour the games depend on (LOW): read the DO polling in the seven entries and the datasheet.
- 93C46 power-up write-disabled (MEDIUM; from datasheet summaries): confirm from the datasheet.
- Light Phaser edge rule and sensor shape (MEDIUM): Phase 5 fixture, with released binaries as oracles.
- `$3E` arbitration (MEDIUM): SMS-SYSTEM section 3, checked against the code.
- libretro firmware keys (MEDIUM): confirm against the current core-info repository.
- Seo Jin and 188-in-1 (LOW): stretch items; ledger rows say what fails.
- Owner titles missing from the MAME lists (MEDIUM): the local `scripts/library.sh` run reports unidentified dumps; names stay local.
- Directory fsync and stale-temp handling in `runner/save.c` (conflicting claims): read the file before Phase 3 is planned.
- PAL rows need owner-held PAL carts; the CC0 lists give no TV field.

**Process gaps:**
- The RetroArch test budget is 299 of 300 lines; ARCHITECTURE's "one slot" figure is wrong. Reclaim lines before adding the round trip.
- `release.firmware_scan` and `local.firmware_window_scan` do not exist and must land first in Phase 6.

### Standing Constraints for Every Phase

- Integer-only, deterministic and bounded. The core never reads a clock; device timing is in ticks; every loop is bounded by emulated time.
- Every behaviour change carries a test that `cmake --workflow --preset ci` runs, and its docs in a file within budget. New behaviour prose goes to `docs/saves.md`, `docs/peripherals.md` or `docs/boards.md`, not to `docs/testing.md` or `docs/embedding.md`.
- Repository-check code is CMake `-P` or POSIX sh and stays under one tenth of `src/` lines (528 of 744 counted now). Real-host (RetroArch) test code stays within 300 lines in total.
- Every gate names the fault it prevents. At most two code-review fix rounds and one gap round per phase; code review runs before verification.
- Each phase re-measures timeouts for the lanes it slows, on its first hosted run, with the run ID in the YAML comment.
- Each phase ends in a release, and the release pull request is not held.
- No GPL, LGPL, non-commercial or unlicensed source is opened or kept. MAME BSD-3 and ISC ares may be read as reference; a port needs a `PROVENANCE.md` row first. GPGX and Gearsystem are used only as released binaries and their documentation.
- No ROM, BIOS, save, BIOS-run frame or BIOS-run state in the tree or in CI artifacts. Owner dumps are judged locally; only SHA-1 digests and predicate results may leave the machine.
- Evidence is a command, an exit status and a run URL, never a committed log.
- Any seed or fixture is pinned to a commit that is public (for example `1d7a454ed79e`).
- API and state bumps are append-only and per phase (ABI stays 1). Pad-only and Sega frame hashes must not move, and the eight-lane hash-equality job is the proof.

## Sources

**Research files for this milestone (2026-11-09):** `.planning/research/STACK.md`, `FEATURES.md`, `ARCHITECTURE.md`, `PITFALLS.md`.

**Repository, read at `b9c6eb7`:** `include/cogmaster.h`; `src/` (`bus`, `io`, `mapper`, `cart`, `cartdb`, `battery`, `boot`, `vdp`, `state`, `instance`, `frame`); `tools/cartdb.c`; `runner/save.c`; `runner/input.c`; `libretro/libretro.c`; `libretro/cogmaster_libretro.info.in`; `tests/cmake/budgets.cmake`; `tests/cmake/repository.sh`; `scripts/compat-common.sh`; `tests/compat/ledger.csv`; `tests/perf/allow.txt`; `.github/workflows/ci.yml`; `docs/compatibility.md`.

**Preparation files (`.planning/preparation/`):** `SMS-CARTRIDGES-AND-PERIPHERALS.md` (sections 2, 3, 4, 6, 8); `SMS-SYSTEM.md` (sections 3, 6, 7); `ARCHITECTURE.md`; `HOSTS-AND-INTEGRATION.md`; `ASSET_POLICY.md` (section 4); `PROVENANCE.md`; `DECISIONS.md`; `ENGINEERING.md` (sections 4, 7, 8); `CONFORMANCE.md` (section 5); seeds SEED-001, SEED-006 and SEED-007.

**External, read under project rule 1:** MAME at `1d7a454ed79e`: the CC0 software lists `hash/sms.xml`, `hash/gamegear.xml` and `hash/sg1000.xml`; the BSD-3 files `src/devices/machine/eepromser.cpp`, `src/devices/bus/sega8/rom.cpp`, `sega8_slot.cpp`, `src/devices/bus/sms_ctrl/*.cpp` and `src/mame/sega/sms.cpp`. ares (ISC) is cited where the research files cite it. libretro documentation for Gearsystem and Genesis Plus GX (documentation only; source not opened). Microchip 93AA46, 93LC46 and 93C46 datasheet summaries (full datasheet not yet read).

**Tertiary, LOW confidence, not relied on for any decision:** web pages on Light Phaser title lists (Sega Retro, Sega Fandom, MobyGames) and on the GG EEPROM titles (GameTech wiki, Black Falcon); tracker issues for MiSTer SMS and Genesis Plus GX.

---
*Research completed: 2026-11-09*
*Ready for roadmap: yes, with the three owner decisions under Gaps (defaults given)*
