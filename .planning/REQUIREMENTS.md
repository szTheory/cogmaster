# Requirements: cogmaster

**Defined:** 2026-10-18
**Core Value:** Master System and Game Gear games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.

## v1 Requirements

Milestone v1.0. Each category is one phase; its pull request builds and rehearses the release archives (FRAME-06), and
merging it publishes them, which is confirmed at ship (AGENTS rule 6). A platform is one of six: Linux x64, Linux arm64, macOS arm64, macOS x64, Windows x64, Windows
arm64; "every platform" means all six plus the big-endian lane. Expected values in tests cite their source document
(`.planning/preparation/*`). Acceptance is on exit status and tolerant regexes, never on exact tool output.

### A test frame in RetroArch

- [x] **FRAME-01**: `cmake --workflow --preset ci` (`ci-msvc` on Windows) configures, builds, tests and packages the library, `cogmaster-run` and the libretro core on every platform, with the exact test inventory in `tests/inventory/ci.txt` executed once each and no skipped test.
- [x] **FRAME-02**: `cogmaster-run --frames 60 --hash` with no cartridge prints a frame hash of the built-in test card (SMS 256×192 and, with `--machine gg`, GG 160×144 showing all 4,096 RGB444 colours) that is byte-identical on every platform, including the `be32` lane under qemu; the report-only `wasm32` lane under wasmtime prints the same hash.
- [x] **FRAME-03**: A CTest loads the libretro core in an in-process host, runs 60 frames, and compares the delivered frame with the runner's byte for byte; `retro_get_system_av_info` reports the NTSC rate 1,640,625/27,379 Hz and the SMS or GG geometry.
- [x] **FRAME-04**: `nm`-based CTests prove the core exports only `cogmaster_*` symbols, has no mutable globals, imports only the four C memory functions (plus the per-OS helper allowlist), links no floating point, and that `include/cogmaster.h` compiles as C17 and C++17; the header stays under 24 KB and the README under 12 KB.
- [x] **FRAME-05**: `scripts/hygiene.sh --tree`, the title check, the timeout-comment check, the SHA-pin check and the no-self-skip policy run in CI, each with a negative control that fails with its named message.
- [x] **FRAME-06**: The pull request's prerelease rehearsal runs every publish step with the App token and produces exactly 18 version-free archives, `SHA256SUMS` and verified attestations, then deletes itself; a packaging test shows `LICENSE` and `THIRD-PARTY-NOTICES.md` in each archive; release-please is configured so that merging publishes the same set with no manual step (the real publish is confirmed at ship).
- [x] **FRAME-07**: The README install line, run in CI against the built macOS arm64 libretro archive (and against `releases/latest/download/` once a release exists), verifies the checksum and installs the core and its `.info` where RetroArch 1.22.2 (Metal) finds it; a hosted macOS job runs RetroArch with the core and no content and compares its screenshot with the runner's frame, with a one-byte negative control.
- [x] **FRAME-08**: An installed-package C consumer, a C++ header consumer and an `add_subdirectory` embed (with tests, runner and libretro off) build in CI on every platform.
- [x] **FRAME-09**: `cogmaster-run --bench` reports frame-time p50/p99/p99.9/p99.99/p99.999 and max RSS; a Cachegrind instructions-per-frame measurement runs on every pull request against its merge base with the +1 % gate present and report-only until two zero-spread control runs arm it (the budgeted `ci:` follow-up, GSD-HANDOFF 3 step 8); a test proves zero allocations after load and that the core never reads a host clock.

### The CPU matches the public vectors

- [x] **CPU-01**: `ctest -L vectors` runs the committed sample of the SingleStepTests `z80` set (the first 100 tests of each of the 1,604 files at `ebe1875d48f3`) and every test matches on final state (including MEMPTR, Q, R, IFF1/2, IM) and on every bus cycle; a nightly lane runs the full set.
- [x] **CPU-02**: The bus is driven by the Z80 on the VDP-clock tick lattice (3 ticks per T-state, 684 per line); a test shows every bus access stamped at its data T-state and `run_frame` returning within one frame plus at most 69 ticks for a HALT/DI loop and for a hostile write loop.
- [x] **CPU-03**: The Sega mapper (`$FFFC`–`$FFFF`, slot 0 first 1 KB fixed, on-cart RAM, bank shift), the no-mapper 32/48 KB layout and the Codemasters mapper pass register-level tests and the `mapper_sega` and `mapper_codemasters` fixtures built in CI with pinned WLA-DX v10.7; images are padded to 16 KB multiples and banks wrap.
- [x] **CPU-04**: Ports `$3E`, `$3F`, `$7E`, `$7F`, `$BE`, `$BF`, `$DC`, `$DD` and GG `$00`–`$06` decode per machine profile, region and TV detection paths return the profile's values, and the Pause NMI fires at the start of the last line; the `ports` fixture reports the values through the RAM mailbox.
- [x] **CPU-05**: `cogmaster-run --probe` prints header fields, region, size, mapper guess, SHA-1, SHA-256, CRC-32 and the RetroAchievements MD5; `cogmaster-run --trace first-fault` prints the first fault (PC, opcode, address, width, device, frame, line, dot) for a ROM that touches an unmapped device; the loader fuzz target runs in a nightly lane and a bounded `fuzz.run` regression replays its seeds per PR.

### Games play in RetroArch

- [ ] **GAME-01**: VDP mode 4 renders name table, 8×8 and 8×16 sprites, zoom, priority, flips, the left-column mask, scroll locks and the 192/224/240-line modes; frame hashes of the `boot_to_play` and `vdp_lines` fixtures are identical on every platform.
- [x] **GAME-02**: Frame and line interrupts, the H and V counters (NTSC and PAL tables) and the status flags follow SMS-VDP 4 at dot-exact ticks; the `vdp_lines` fixture's mailbox values match `expected.csv` and a test shows a status read that clears the flags.
- [x] **GAME-03**: The `gg` profile renders the 160×144 window with 12-bit CRAM, GG ports, Start and the SMS-compat mode; `boot_to_play.gg` plays with the same oracle as the SMS fixture, and the hosted job's GG screenshot matches the runner's crop.
- [ ] **GAME-04**: Two pads, Pause and region/TV profiles work end to end: the `boot_to_play` fixtures prove that a button press changes a visible counter, a scroll register and the frame hash, and that a frame with no port read is counted as a lag frame.
- [ ] **GAME-05**: Battery RAM is exported as a raw span with a generation counter; `cogmaster-run --save-dir` writes atomically on exit and on SIGTERM; the `battery` fixture continues from its saved value in a fresh process, with missing-save and wrong-save controls that differ.
- [ ] **GAME-06**: `tests/compat/ledger.csv` and the generated `docs/compatibility.md` exist with the fixture rows and the ratchet check; the local `library` preset (never registered in `ci`) runs every image in `COGMASTER_ROM_DIR` against the ledger and writes only locally, shown by a test that points it at a fixture in a temporary directory; the libretro `.info` carries `is_experimental` until v2.0.
- [x] **GAME-07**: One integrated session in RetroArch (hosted job, ≤ 300 lines of e2e code in total) boots `boot_to_play.sms`, an input changes play, the picture scrolls, and the screenshot matches the runner's frame.

### Sound

- [ ] **SND-01**: `ctest -L psg` steps every tone, volume and noise setting of the SN76489 model (16-bit LFSR, taps 0 and 3, reset on write, period 0 as 1, 2 dB steps) against SMS-AUDIO 1 and JoppyFurr's MIT test ROM, and the GG stereo register routes channels per SMS-AUDIO 2.
- [ ] **SND-02**: The YM2413 plays for the Japanese profile at Z80/72 Hz with port `$F2` selecting PSG/FM/none/both, seeded from the permissive OPLL chosen at phase 4 planning (DEC.22) with its notice, and `ctest -L fm` passes JoppyFurr's MIT YM2413 test ROM checks.
- [ ] **SND-03**: `cogmaster-run --hash-audio` prints transition-stream and PCM hashes for the `psg_sweep` and `boot_to_play` fixtures, identical on every platform, with exactly 3,504,512/4,375 samples per NTSC frame at 48 kHz and no clipping.
- [ ] **SND-04**: The libretro batch and single-sample audio paths deliver PCM equal to the runner's (stereo on GG, duplicated mono on SMS), and the hosted RetroArch session records non-silent audio.
- [ ] **SND-05**: A test renders reference tones through the synthesiser and shows the largest non-harmonic peak below 16 kHz under −80 dB; synthesis cost per output sample stays bounded under a register-hammering fixture.

## v2 Requirements

Later milestones. Each has a seed in `.planning/seeds/` that says when it comes back.

### The whole library (SEED-001)

- **LIB-01**: Korean mappers (MSX 4PAK, Nemesis, Janggun, Hicom, Zemina, 188-in-1, Seojin), the 93C46 EEPROM and every other board in MAME's CC0 lists, detected by hash and heuristics.
- **LIB-02**: Light Phaser from the exact beam position, 3-D glasses shutter state per frame, Sports Pad, Paddle, Mega Drive pad.
- **LIB-03**: BIOS boot from the user's dump by digest; Japanese FM, PAL and Korean/Brazilian profiles over the whole ledger; `.srm` interop measured against other emulators; the libretro `.info` drops `is_experimental`.

### States and movies (SEED-002)

- **STATE-01**: Fixed-size, CRC-checked save states validated before apply, fork-tested at every frame, with the libretro `serialized` and `deterministic` claims behind their tests.
- **STATE-02**: An own text movie format with lag frames and BK2/MMV import; rewind and run-ahead in RetroArch; Playstead continuation.

### Measured accuracy (SEED-003)

- **ACC-01**: Diagnostic ROMs run on the owner's consoles settle the open constants (DECISIONS open questions); nightly oracle lanes against released binaries; the per-dot VDP model where measured.

### Tools (SEED-004)

- **TOOL-01**: Hooks at zero cost when off, WLA-DX `.sym` loading, `--patch` IPS/BPS, Python bindings and a Gymnasium environment, Lua on demand.

### Everywhere (SEED-005) and the browser (SEED-008)

- **PORT-01**: Compile lanes for console and handheld SDKs, the libretro buildbot recipe, Playstead `sms`/`gg` adapter pin.
- **WEB-01**: An Emscripten build with a static page whose hashes equal native.

### SG-1000 and SC-3000 (SEED-006)

- **SG-01**: TMS9918 modes 0–3 with the SMS VDP's documented differences, the SG-1000 memory map, the SC-3000 keyboard and the SG mappers; until then such images are refused with "unsupported: SG-1000 (planned)".

## Out of Scope

| Feature | Reason |
|---|---|
| A window, audio device or input device in this repository | Hosts own presentation (DEC.01) |
| Shaders, menus, cheats, netplay UI | Frontend features; the core exposes what they need |
| Any Sega BIOS or game bytes | Copyright (ASSET_POLICY) |
| GPL, LGPL or non-commercial code | MIT licence (PROVENANCE) |
| A fast or low-accuracy mode that changes timing | Exact timing is the product (DEC.05) |
| Signed macOS builds | A Developer ID certificate puts a legal name in artifacts; ad-hoc signing only |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| FRAME-01 | Phase 1 | Complete |
| FRAME-02 | Phase 1 | Complete |
| FRAME-03 | Phase 1 | Complete |
| FRAME-04 | Phase 1 | Complete |
| FRAME-05 | Phase 1 | Complete |
| FRAME-06 | Phase 1 | Complete |
| FRAME-07 | Phase 1 | Complete |
| FRAME-08 | Phase 1 | Complete |
| FRAME-09 | Phase 1 | Complete |
| CPU-01 | Phase 2 | Complete |
| CPU-02 | Phase 2 | Complete |
| CPU-03 | Phase 2 | Complete |
| CPU-04 | Phase 2 | Complete |
| CPU-05 | Phase 2 | Complete |
| GAME-01 | Phase 3 | Pending |
| GAME-02 | Phase 3 | Complete |
| GAME-03 | Phase 3 | Complete |
| GAME-04 | Phase 3 | Pending |
| GAME-05 | Phase 3 | Pending |
| GAME-06 | Phase 3 | Pending |
| GAME-07 | Phase 3 | Complete |
| SND-01 | Phase 4 | Pending |
| SND-02 | Phase 4 | Pending |
| SND-03 | Phase 4 | Pending |
| SND-04 | Phase 4 | Pending |
| SND-05 | Phase 4 | Pending |

**Coverage:** v1 requirements: 26 total; 26 mapped (Phase 1: 9, Phase 2: 5, Phase 3: 7, Phase 4: 5); 0 orphaned; 0 duplicated.

---
*Requirements defined: 2026-10-18*
*Last updated: 2026-10-18 after preparation review*
