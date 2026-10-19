# Project Research Summary

**Project:** cogmaster
**Domain:** Sega Master System and Game Gear emulator core in C (embeddable library, headless runner, libretro core)
**Researched:** 2026-10-18
**Confidence:** HIGH for structure, stack, delivery and the CPU plan; MEDIUM for a few VDP timing constants that are inferred from emulators rather than measured (named, cited and testable — see Gaps)

## Executive Summary

cogmaster is the owner's sixth emulator and inherits a proven shape: a deterministic, integer-only C17 core with a
headless runner and a libretro adapter, shipped by an automated pipeline from the first phase. The Master System and
Game Gear share a Z80, a TMS9918-derived VDP, the SN76489 PSG and Sega's paging chip, so one core with machine
profiles covers both. Three facts make this console cheaper than the siblings: an MIT per-cycle Z80 vector corpus
(SingleStepTests, 1,604 files), a permissive reference emulator (ares, ISC) and BSD-3 MAME sources that may be read,
and CC0 hash databases that give mapper and region facts for 653 SMS and 823 GG titles. Three boards (none, Sega,
Codemasters) cover 97 % of that library.

The sibling evidence is equally clear about what to avoid: self-invented verification ceremony, real-host test
harnesses that outgrow the emulator, release qualification deferred to milestone end, and fixture provenance on the
critical path. Every rule that came from those failures is in `AGENTS.md`, and every lesson is a ledger row in
`.planning/LESSONS.md`.

## Key Findings

### Recommended Stack
See `STACK.md`; details, pins and the rule → check table in `.planning/preparation/ENGINEERING.md`.

### Expected Features
v1.0: the delivery path with a test card in RetroArch; the whole Z80 against the public vectors; Master System and
Game Gear games playing in RetroArch with two pads, regions, battery saves, self-authored fixtures and a
compatibility ledger; the PSG with GG stereo and the YM2413. Later: every mapper and peripheral, BIOS boot, PAL and
FM over the ledger (v2.0); states, rewind, run-ahead, movies, Playstead continuation (v3.0); hardware-measured
accuracy (v4.0); hooks, debugger views, Python and Gymnasium (v5.0); handheld and console ports, the libretro
buildbot, the browser (v6.0 and SEED-008); SG-1000/SC-3000 mode (SEED-006).

### Architecture Approach
One `uint64_t` tick per VDP clock; the Z80 drives the bus and stamps each access; devices catch up lazily; the VDP
renders per line with catch-up to the current dot and dot-exact interrupt and latch positions; a frame runs from
frame-interrupt tick to frame-interrupt tick with input applied there; video is one 256×H RGB444 field with a crop
rect; audio is a transition stream with an exact rational sample count per frame; state is a four-pass visitor;
observability is always-on counters plus a first-fault record. Details in `.planning/preparation/ARCHITECTURE.md`.

**Major components:**
1. **Core library** (`cogmaster`): z80, bus, mapper, vdp, psg, opll, io, cart loader and hash DB, boot, synth, state, instance.
2. **Runner** (`cogmaster-run`): frames, hashes, PNG, saves, probe, first-fault trace, bench; the automation and Playstead surface.
3. **libretro core**: the adapter over the library with GPGX-compatible pad map, BIOS names and option keys.

### Critical Pitfalls
1. Growing the CPU opcode by opcode from game traces instead of the vectors (a sibling never booted a game).
2. Real-host e2e and verification machinery that outgrows the emulator (10k test lines, no `src/` change).
3. Deferring releases to a milestone-end qualification phase (eight phases merged unpublished).
4. Fixture and toolchain provenance on the critical path (8 of 13 plans); hence WLA-DX pinned by digest and own fixtures.

## Implications for Roadmap

Milestone heading for ROADMAP.md: `## v1.0 SMS and Game Gear games play with sound in RetroArch`. Four phases. Merging each one publishes a release.

### Phase 1: A test frame in RetroArch
**Rationale:** Ship the whole delivery path before any emulation exists, so every later phase ends in a release by merging; the test card proves the frame contract on every platform.
**Delivers:** The library with its public header and test cards (SMS 256×192, GG 160×144), `cogmaster-run` with hashes and bench, the libretro core, six-platform CI plus big-endian and wasm32 lanes, hygiene and policy gates with negative controls, release-please with the App token, the rehearsal, 18 attested archives, the README install line, the hosted RetroArch screenshot compare, installed and embedded consumers, the Ir/frame measurement.
**Addresses:** FRAME-01 to FRAME-09
**UI hint**: no
**Canonical refs:** `.planning/preparation/ENGINEERING.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/preparation/ARCHITECTURE.md`, `PROVENANCE.md`

### Phase 2: The CPU matches the public vectors
**Rationale:** Public vectors judge the Z80 on its own before any other chip exists; the bus timing, the mappers, the ports and the Pause NMI come with it, and the first-fault trace makes every later stall diagnosable.
**Delivers:** An own Z80 (documented and undocumented behaviour, MEMPTR, Q, R, interrupt timing) matching the committed SingleStepTests sample on every cycle with the full set nightly; the tick-lattice bus with bounded frames; the Sega, no-mapper and Codemasters boards with WLA-DX-built fixtures; ports, region and TV detection, Pause; `--probe` and `--trace first-fault`; the loader fuzz target.
**Addresses:** CPU-01 to CPU-05
**UI hint**: no
**Canonical refs:** `.planning/preparation/SMS-SYSTEM.md`, `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/SMSPOWER-EXTRACTS.md`

### Phase 3: Games play in RetroArch
**Rationale:** With the CPU and bus proven, the VDP, input and saves turn fixtures into playable games on both machines; the ledger starts ratcheting with the owner's carts.
**Delivers:** VDP mode 4 with dot-exact interrupts, counters and flags; the GG window, palette and SMS-compat mode; two pads, Pause, regions; battery saves with atomic runner writes and fresh-process continuation; the `boot_to_play`, `vdp_lines` and `battery` fixtures on SMS and GG; the ledger and generated compatibility page; the local `library` preset; one integrated RetroArch session.
**Addresses:** GAME-01 to GAME-07
**Uses:** the first-fault trace and fixtures from Phase 2
**UI hint**: no
**Canonical refs:** `.planning/preparation/SMS-VDP.md`, `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`

### Phase 4: Sound
**Rationale:** Sound last, because its oracle (transition hashes, MIT test ROMs, a hosted non-silent recording) needs a playing game, and because the FM seed is the one third-party module of v1.0.
**Delivers:** The SN76489 model with GG stereo, the YM2413 for the Japanese profile seeded with its notice, integer band-limited synthesis at an exact rational rate, transition and PCM hashes on every platform, libretro batch and single-sample paths, the spectral test.
**Addresses:** SND-01 to SND-05
**UI hint**: no
**Canonical refs:** `.planning/preparation/SMS-AUDIO.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`

### Phase Ordering Rationale
Delivery first so nothing is ever unreleased; CPU against an exhaustive oracle before any chip that depends on it;
video, input and saves together because one integrated play check proves all three; sound last because its
evidence needs a playing game. Each phase opens with a tracer plan through every layer before widening, and plans
that do not share a test inventory run in parallel waves.

### Research Flags
Phases likely needing deeper research during planning: Phase 3 (VDP timing conflicts in SMS-VDP 10 and ARCHITECTURE 13; GG SMS-compat mode), Phase 4 (OPLL seed choice and FM:PSG mix).
Phases with standard patterns: Phase 1 (muhtari's pipeline, renamed), Phase 2 (vectors harness pattern from muhtari and nesturbator).

## Confidence Assessment

| Area | Confidence | Notes |
|---|---|---|
| Stack | HIGH | Sibling-proven; dated facts re-verified 2026-10-18 |
| Features | HIGH | Library shares counted from CC0 lists; complaints mapped from issue trackers |
| Architecture | HIGH | Measured clock chain; API conventions held three sibling milestones |
| Pitfalls | HIGH | Drawn from five siblings' histories, with revisions |
| VDP timing constants | MEDIUM | Logic-analyser figures conflict with two emulators on a few ticks; fixtures and v4.0 hardware runs settle them |

**Overall confidence:** HIGH

### Gaps to Address
- Frame/line interrupt tick, vertical scroll latch, SMS1 zoom slot rule, open-bus byte (DECISIONS open questions): settled by Phase 3 fixtures against released binaries, then hardware in v4.0.
- devkitSMS crt0 linking exception: quoted and judged at the first third-party fixture admission (Phase 3).
- SMS Power! was returning 503 during preparation; the facts taken from archived captures are flagged in SMSPOWER-EXTRACTS and re-read at the Phase 2 and 3 research steps.

## Sources
Tiered and pinned in each preparation file's Sources table; the index is `.planning/preparation/README.md`.

---
*Research completed: 2026-10-18*
*Ready for roadmap: yes*
