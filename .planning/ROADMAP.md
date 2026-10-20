# Roadmap: cogmaster

## Overview

cogmaster ships its whole delivery path before any emulation exists: a test card drawn by the library, hashed by
`cogmaster-run`, delivered by the libretro core and photographed in a hosted RetroArch, released on six platforms by
merging one pull request. The Z80 is then proven against the public SingleStepTests vectors on every cycle, together
with the tick-lattice bus, the three mappers that cover 97 % of the library, the ports and the Pause NMI. With the CPU
and bus trusted, the VDP, two pads, regions and battery saves turn self-authored fixtures into playable Master System
and Game Gear games in RetroArch, and the compatibility ledger starts ratcheting. Sound comes last because its oracle
needs a playing game: the SN76489 with Game Gear stereo, the YM2413 for the Japanese profile, and frame and audio
hashes identical on every platform. Every phase is one branch, one pull request and one automated release.

## v1.0 SMS and Game Gear games play with sound in RetroArch

**Milestone goal:** Master System and Game Gear games play with sound in RetroArch from a small MIT-licensed C
library, with the Z80 vector-exact, frame and audio hashes identical on every platform, and a release published by
merging each phase.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: A test frame in RetroArch** - The library, runner and libretro core deliver a hashed test card on every platform, installed into RetroArch by the README line, released by merging (completed 2026-10-19)
- [x] **Phase 2: The CPU matches the public vectors** - An own Z80 matches SingleStepTests on every cycle, with the tick-lattice bus, the Sega and Codemasters mappers, ports, Pause and the first-fault trace (completed 2026-10-20)
- [ ] **Phase 3: Games play in RetroArch** - VDP mode 4, the Game Gear window, two pads, regions and battery saves make fixtures playable on both machines, with the ledger ratcheting
- [ ] **Phase 4: Sound** - The PSG with Game Gear stereo and the YM2413 play through the runner and RetroArch with transition and PCM hashes identical on every platform

## Phase Details

### Phase 1: A test frame in RetroArch

**Goal**: A host can build, install and run cogmaster on every platform and get the same test-card frame from the library, the runner and the libretro core, before any emulation exists; merging the pull request publishes the release.
**Depends on**: Nothing (first phase)
**Requirements**: FRAME-01, FRAME-02, FRAME-03, FRAME-04, FRAME-05, FRAME-06, FRAME-07, FRAME-08, FRAME-09
**Success Criteria** (what must be TRUE):
  1. `cmake --workflow --preset ci` (`ci-msvc` on Windows) exits 0 on all six platforms, and a test shows the exact inventory in `tests/inventory/ci.txt` ran once each with no skipped test (FRAME-01).
  2. `cogmaster-run --frames 60 --hash` with no cartridge prints the same SMS test-card hash, and with `--machine gg` the same GG test-card hash, on all six platforms, on the `be32` lane under qemu and on the `wasm32` lane under wasmtime (FRAME-02).
  3. A CTest loads the libretro core in an in-process host, runs 60 frames, and the delivered frame equals the runner's byte for byte; `retro_get_system_av_info` reports 1,640,625/27,379 Hz and the SMS or GG geometry (FRAME-03).
  4. The pull request's prerelease rehearsal produces exactly 18 version-free archives, `SHA256SUMS` and verified attestations and then deletes itself; a packaging test finds `LICENSE` and `THIRD-PARTY-NOTICES.md` in each archive (FRAME-06).
  5. The README install line, run in CI against the built macOS arm64 libretro archive, verifies the checksum and installs the core and its `.info` where RetroArch 1.22.2 finds it; the hosted macOS job's RetroArch screenshot equals the runner's frame, and its one-byte negative control fails (FRAME-07).

**Plans**: 12/12 plans complete, 01-01 to 01-12, one sequential chain (waves 1–12)
- [x] 01-01-PLAN.md
- [x] 01-02-PLAN.md
- [x] 01-03-PLAN.md
- [x] 01-04-PLAN.md
- [x] 01-05-PLAN.md
- [x] 01-06-PLAN.md
- [x] 01-07-PLAN.md
- [x] 01-08-PLAN.md
- [x] 01-09-PLAN.md
- [x] 01-10-PLAN.md
- [x] 01-11-PLAN.md
- [x] 01-12-PLAN.md

**UI hint**: no
**Canonical refs:** `.planning/preparation/ENGINEERING.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`, `.planning/preparation/ARCHITECTURE.md`, `PROVENANCE.md`

### Phase 2: The CPU matches the public vectors

**Goal**: The Z80, the tick-lattice bus, the three v1.0 boards, the ports and the Pause NMI are proven against public vectors and own fixtures before any game runs, and every later stall is diagnosable from the first-fault trace.
**Depends on**: Phase 1
**Requirements**: CPU-01, CPU-02, CPU-03, CPU-04, CPU-05
**Success Criteria** (what must be TRUE):
  1. `ctest -L vectors` passes: the first 100 tests of each of the 1,604 SingleStepTests `z80` files at `ebe1875d48f3` match on final state (including MEMPTR, Q, R, IFF1/2, IM) and on every bus cycle, and the nightly lane runs the full set (CPU-01).
  2. A test shows every bus access stamped at its data T-state on the 3-ticks-per-T-state, 684-ticks-per-line lattice, and `run_frame` returns within one frame plus at most 69 ticks for a HALT/DI loop and for a hostile write loop (CPU-02).
  3. The Sega, no-mapper and Codemasters boards pass their register-level tests, and the `mapper_sega` and `mapper_codemasters` fixtures, built in CI with pinned WLA-DX v10.7, report the expected values through the RAM mailbox, with images padded to 16 KB multiples and banks wrapping (CPU-03).
  4. The `ports` fixture reports the profile's values for ports `$3E`, `$3F`, `$7E`, `$7F`, `$BE`, `$BF`, `$DC`, `$DD` and GG `$00`–`$06` per machine profile and region, and a test shows the Pause NMI firing at the start of the last line (CPU-04).
  5. `cogmaster-run --probe` prints header fields, region, size, mapper guess, SHA-1, SHA-256, CRC-32 and the RetroAchievements MD5; `cogmaster-run --trace first-fault` prints the first fault (PC, opcode, address, width, device, frame, line, dot) for a ROM that touches an unmapped device; the bounded `fuzz.run` regression replays the loader fuzz seeds per PR (CPU-05).

**Plans**: 17/17 plans complete, 02-01 to 02-17, one sequential chain (waves 1–17; every plan registers tests in the shared inventories)
**Wave 1**
- [x] 02-01-PLAN.md — Vector pipeline: stream builder, sst2bin, committed sample, harness, NOP and LD (BC),A

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 02-02-PLAN.md — Vector negative control on a corrupted copy, allow list, converter tests, conformance page

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 02-03-PLAN.md — Z80 main page and the vectors.base shard

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 02-04-PLAN.md — CB page, DD/FD prefix steps, DDCB/FDCB and five shards

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 02-05-PLAN.md — ED page, block ops, holes, vectors.ed; interrupt, NMI, EI-delay and prefix-run tests

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 02-06-PLAN.md — Images load and run bounded: bus lattice, loader size rules, fast boot, frame origin, CPU state

**Wave 7** *(blocked on Wave 6 completion)*
- [x] 02-07-PLAN.md — API version 2 region and TV: PAL frame, profile rejection, info fields

**Wave 8** *(blocked on Wave 7 completion)*
- [x] 02-08-PLAN.md — VDP timing half, frame interrupt, counters, ports, Pause NMI tick, stamps and hostile loop

**Wave 9** *(blocked on Wave 8 completion)*
- [x] 02-09-PLAN.md — cogmaster_probe and the Sega, Codemasters and no-mapper boards

**Wave 10** *(blocked on Wave 9 completion)*
- [x] 02-10-PLAN.md — I/O ports per profile and region; first-fault call sites

**Wave 11** *(blocked on Wave 10 completion)*
- [x] 02-11-PLAN.md — Runner: FILE, --machine auto, --region, --probe digests, exit 77, --trace first-fault

**Wave 12** *(blocked on Wave 11 completion)*
- [x] 02-12-PLAN.md — Pinned WLA-DX build, hello fixture, pin self test, fixture digests across lanes

**Wave 13** *(blocked on Wave 12 completion)*
- [x] 02-13-PLAN.md — cogmaster_peek, --mailbox, CSV judge and its controls, mapper_sega paging fixture

**Wave 14** *(blocked on Wave 13 completion)*
- [x] 02-14-PLAN.md — mapper_codemasters fixture, full mapper_sega records, corrected phase matrix

**Wave 15** *(blocked on Wave 14 completion)*
- [x] 02-15-PLAN.md — ports fixture under sms-export, sms-japan, sms-pal and gg

**Wave 16** *(blocked on Wave 15 completion)*
- [x] 02-16-PLAN.md — Fuzz entries, per-PR replay, nightly libFuzzer

**Wave 17** *(blocked on Wave 16 completion)*
- [x] 02-17-PLAN.md — Nightly full vectors, measured timeouts, all-legs CI run, maintainer docs

**UI hint**: no
**Canonical refs:** `.planning/preparation/SMS-SYSTEM.md`, `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/SMSPOWER-EXTRACTS.md`

### Phase 3: Games play in RetroArch

**Goal**: Master System and Game Gear fixtures boot, draw, scroll, respond to two pads and keep battery saves across processes, in the runner and in one integrated RetroArch session, with frame hashes identical on every platform and a compatibility ledger that only ratchets up.
**Depends on**: Phase 2
**Requirements**: GAME-01, GAME-02, GAME-03, GAME-04, GAME-05, GAME-06, GAME-07
**Success Criteria** (what must be TRUE):
  1. The frame hashes of the `boot_to_play` and `vdp_lines` fixtures are identical on every platform, exercising name table, 8×8 and 8×16 sprites, zoom, priority, flips, the left-column mask, scroll locks and the 192/224/240-line modes (GAME-01).
  2. `boot_to_play.gg` plays under the `gg` profile with the same oracle as the SMS fixture, rendering the 160×144 window with 12-bit CRAM, and the hosted job's GG screenshot equals the runner's crop (GAME-03).
  3. A test shows that a button press on either pad in `boot_to_play` changes a visible counter, a scroll register and the frame hash, and that a frame with no port read is counted as a lag frame (GAME-04).
  4. The `battery` fixture continues from its saved value in a fresh `cogmaster-run --save-dir` process after exit and after SIGTERM, and the missing-save and wrong-save controls produce a different result (GAME-05).
  5. One hosted RetroArch session, with at most 300 lines of end-to-end code in total, boots `boot_to_play.sms`, an input changes play, the picture scrolls, and the screenshot equals the runner's frame (GAME-07).

**Plans**: 11/18 plans executed, one sequential chain (waves 1–18; shared source and exact inventories)

Plans:
**Wave 1**
- [x] 03-01-PLAN.md — Owned SMS rendering/input tracer through library, runner and content-loaded libretro

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 03-02-PLAN.md — Consolidate repository checks and enforce the strict source ratio

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 03-03-PLAN.md — Hosted SMS input, scroll and screenshot proof within 300 lines

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 03-04-PLAN.md — Complete backgrounds, scroll locks and shared SMS1 profile masks

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 03-05-PLAN.md — Bounded sprites, zoom, priority, collision and overflow

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 03-06-PLAN.md — IRQ pending/clear races, all counters and TH capture

**Wave 7** *(blocked on Wave 6 completion)*
- [x] 03-07-PLAN.md — Coherent completed heights and the vdp_lines fixture

**Wave 8** *(blocked on Wave 7 completion)*
- [x] 03-08-PLAN.md — Native GG CRAM/crop/freeze, SMS mode and playing fixture

**Wave 9** *(blocked on Wave 8 completion)*
- [x] 03-09-PLAN.md — GG/PAL presentation, geometry notifications and hosted GG crop

**Wave 10** *(blocked on Wave 9 completion)*
- [x] 03-10-PLAN.md — Two pads, Pause/Start, Reset/Restart, disconnect and lag

**Wave 11** *(blocked on Wave 10 completion)*
- [x] 03-11-PLAN.md — Pinned CC0 metadata, fixture identity and physical geometry disposition

**Wave 12** *(blocked on Wave 11 completion)*
- [ ] 03-12-PLAN.md — Exact battery spans, mapped-write generations and transactional import

**Wave 13** *(blocked on Wave 12 completion)*
- [ ] 03-13-PLAN.md — Native atomic .srm replacement and signal/failure behavior

**Wave 14** *(blocked on Wave 13 completion)*
- [ ] 03-14-PLAN.md — Fresh-process battery continuation and stable frontend memory

**Wave 15** *(blocked on Wave 14 completion)*
- [ ] 03-15-PLAN.md — Bounded predicates, quoted CSV ledger and generated compatibility docs

**Wave 16** *(blocked on Wave 15 completion)*
- [ ] 03-16-PLAN.md — Protected normalized ratchet, dated exceptions and trusted base judge

**Wave 17** *(blocked on Wave 16 completion)*
- [ ] 03-17-PLAN.md — Every-candidate local library and filtered public updates

**Wave 18** *(blocked on Wave 17 completion)*
- [ ] 03-18-PLAN.md — Playing hashes, boundedness, representative workload and review handoff

**UI hint**: no
**Canonical refs:** `.planning/preparation/SMS-VDP.md`, `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md`, `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/HOSTS-AND-INTEGRATION.md`

### Phase 4: Sound

**Goal**: Games play with sound: the SN76489 with Game Gear stereo and the YM2413 for the Japanese profile are synthesised at an exact rational rate, delivered through the runner and both libretro audio paths, with transition and PCM hashes identical on every platform.
**Depends on**: Phase 3
**Requirements**: SND-01, SND-02, SND-03, SND-04, SND-05
**Success Criteria** (what must be TRUE):
  1. `ctest -L psg` passes every tone, volume and noise setting of the SN76489 model against SMS-AUDIO 1 and JoppyFurr's MIT test ROM, and the GG stereo register routes channels per SMS-AUDIO 2 (SND-01).
  2. `ctest -L fm` passes JoppyFurr's MIT YM2413 test ROM checks for the Japanese profile at Z80/72 Hz, with port `$F2` selecting PSG, FM, none or both, and the OPLL seed's notice is present in `THIRD-PARTY-NOTICES.md` (SND-02).
  3. `cogmaster-run --hash-audio` prints the same transition-stream and PCM hashes for the `psg_sweep` and `boot_to_play` fixtures on every platform, with exactly 3,504,512/4,375 samples per NTSC frame at 48 kHz and no clipping (SND-03).
  4. A test shows the libretro batch and single-sample audio paths deliver PCM equal to the runner's (stereo on GG, duplicated mono on SMS), and the hosted RetroArch session's recording is non-silent (SND-04).
  5. A test renders reference tones through the synthesiser and shows the largest non-harmonic peak below 16 kHz under −80 dB, and synthesis cost per output sample stays bounded under a register-hammering fixture (SND-05).

**Plans**: TBD
**UI hint**: no
**Canonical refs:** `.planning/preparation/SMS-AUDIO.md`, `.planning/preparation/ARCHITECTURE.md`, `.planning/preparation/CONFORMANCE.md`

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. A test frame in RetroArch | 12/12 | Complete    | 2026-10-19 |
| 2. The CPU matches the public vectors | 17/17 | Complete    | 2026-10-20 |
| 3. Games play in RetroArch | 11/18 | In Progress | - |
| 4. Sound | 0/TBD | Not started | - |
