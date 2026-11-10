# cogmaster

## What This Is

A Sega Master System and Game Gear emulator core written in C17: a small MIT-licensed library, a headless runner
(`cogmaster-run`) for tests, automation and frontends such as Playstead, and a libretro core for RetroArch. One
core covers the Mark III, both Master System revisions, the Japanese FM unit and the Game Gear as machine
profiles. It is for host builders who want an embeddable, deterministic core; players who want exact timing and
sound; homebrew developers and ROM hackers who want inspection and tooling; speedrunners, TAS authors and AI
researchers who need frame-exact, reproducible runs; and porters who need an integer-only core that runs on
handhelds and in the browser. The owner tries every release on a Mac in RetroArch and tests with a large
collection of physical cartridges.

## Current Milestone: v2.0 The whole library

**Goal:** The cartridges and peripherals a collector owns all play, with saves and the PAL and Japan profiles,
checked against the whole CC0 MAME ledger.

**Target features:**
- Tune-up first (SEED-007): the v1.0 debt, measured CI timeouts, the perf gate armed, the README's ROM step, the weekly triage digest and `quality.json`
- The remaining mappers (Korean boards) with hash-database detection
- Saves: 93C46 EEPROM, `.srm` interop, runner `--save-dir` with atomic writes
- Peripherals: Light Phaser, Sports Pad, Paddle, 3-D glasses, and the BIOS boot path with the user's own dump
- Full PAL and Japanese FM profiles over the ledger, and the ledger over the whole CC0 MAME lists, after which `is_experimental` goes

Browser (SEED-008) and SG-1000/SC-3000 (SEED-006) stay dormant.

## Core Value

Master System and Game Gear games behave as they do on the console, identically on every platform, from a small
MIT-licensed C library that any host can embed.

## Requirements

### Validated

- ✓ Release automation from the first phase: library, runner and libretro core built, tested, attested and published on every platform by merging a pull request; one install line puts the core into RetroArch on a Mac — Phase 1
- ✓ The Z80 matches the public SingleStepTests vectors on every cycle before any game runs, with the bus, Sega and Codemasters mappers, ports and the Pause NMI — Phase 2
- ✓ SMS and Game Gear owned fixtures play through the runner and released RetroArch, with two pads, regions, raw battery continuation, identical playing hashes on eight lanes and a protected compatibility ledger; the private-library path is proven with synthetic fixtures — Phase 3
- ✓ Sound: the SN76489 with Game Gear stereo and the YM2413 for the Japanese profile play through the runner and RetroArch, with transition and PCM hashes identical on every platform including the 32-bit big-endian lane and the wasm32 lane — Phase 4

### Active

Milestone v2.0 (scoped in `.planning/REQUIREMENTS.md`):

- [ ] A tune-up that clears the v1.0 CI, perf-gate, review and README debt and adds the triage digest and `quality.json` ratchet
- [ ] The remaining cartridge mappers detected and played
- [ ] Saves that round-trip: 93C46 EEPROM, `.srm` interop and atomic `--save-dir` writes
- [ ] Light Phaser, Sports Pad, Paddle and 3-D glasses at pin level, and the BIOS boot path
- [ ] PAL and Japanese FM over the ledger, and the ledger over the whole CC0 MAME lists

### Out of Scope

- A window, audio device or input device — hosts own presentation; RetroArch is the frontend.
- Shaders, menus, cheats, rewind UI, netplay UI — frontend features; the core exposes what they need.
- Any Sega BIOS or game bytes — copyright; the user's own dumps stay on the user's machine (`ASSET_POLICY.md`).
- GPL, LGPL or non-commercial code — MIT licence (`PROVENANCE.md`).
- A fast or low-accuracy mode that changes timing — exact timing is the product.
- Signed macOS builds — a Developer ID certificate puts a legal name in artifacts.
- Later milestones: the whole library and peripherals, states and movies, measured accuracy, tools, ports, the browser and SG-1000 mode are seeds in `.planning/seeds/` and horizons in `.planning/PRODUCT-ROADMAP.md`.

## Context

- **Ecosystem.** Genesis Plus GX (non-commercial) sits under most frontends; Emulicious (closed) has the best
  debugger; ares (ISC) is the permissive reference; Gearsystem, MEKA and BlastEm are GPL. No permissive,
  embeddable, exact-timing SMS+GG core with first-class tooling exists (`.planning/preparation/SMS-ECOSYSTEM-AND-USERS.md`).
- **Sibling projects.** muhtari (Atari 7800) is the direct template and seed for the runner, libretro adapter,
  state visitor, tests scaffolding, CI, hygiene and release pipeline; nesturbator (NES), gabbaboy (Game Boy),
  glueyneo (Neo Geo), genewhip (Genesis) and Playstead (frontend) supplied the lessons in `.planning/LESSONS.md`.
- **Shipped v1.0** (4 phases, 62 plans, 26 of 26 requirements; releases v0.1.0 to v0.4.0, one per phase): about 7,700
  lines of C in `src/` and `include/`, about 26,000 with tests. The milestone audit found no gaps and recorded tech
  debt (`.planning/milestones/v1.0-MILESTONE-AUDIT.md`): provisional CI timeouts (windows-arm64 failed one main
  run), the perf gate still in report mode with a `boot_to_play` allow line to 2027-01-07, open review
  dispositions from phases 1, 2 and 4, a README that does not yet say how to load a ROM, nightly lanes not yet run
  on the sound code, and third-party sound ROM conformance deferred with no lane.
- **How releases are tried.** The owner installs the libretro core into RetroArch (Metal build) on an Apple
  Silicon Mac with the README's one line and plays a cartridge dump from their own collection.
- **Reference material** (`.planning/preparation/`, index in `README.md` there): `SMS-SYSTEM.md` (clocks, memory,
  ports, regions, BIOS, boot), `SMS-VDP.md` (modes, registers, timing, counters, GG window and palette),
  `SMS-AUDIO.md` (PSG, GG stereo, YM2413, synthesis), `SMS-CARTRIDGES-AND-PERIPHERALS.md` (header, mappers,
  controllers, saves, bounds), `SMS-ECOSYSTEM-AND-USERS.md` (emulators, complaints, personas, budgets),
  `SMSPOWER-EXTRACTS.md` (forum and wiki facts, test ROMs, fixtures, toolchain pin), `ARCHITECTURE.md`,
  `CONFORMANCE.md`, `HOSTS-AND-INTEGRATION.md`, `ENGINEERING.md`, `DECISIONS.md`, `GSD-HANDOFF.md`,
  `PROMPT-COVERAGE.md`; plus `PROVENANCE.md` and `ASSET_POLICY.md` at the root.

## Constraints

- **Deliverables**: library, runner, libretro core; nothing else in this repository.
- **Language**: C17 without extensions; the core links only the C memory functions; the header is valid C++.
- **Reuse**: permissive seeds only, through `PROVENANCE.md` §4; GPL, LGPL, non-commercial and unlicensed source is never opened.
- **Determinism**: integer-only, no globals, no host clock; identical frame and audio hashes on every platform.
- **Efficiency**: handhelds and wasm32 are targets; an instructions-per-frame ratchet from phase 1; zero allocations after load.
- **Legal**: no Sega BIOS or game bytes; fixtures are own WLA-DX assembly or permissively licensed at their pin.
- **Privacy**: public repository; no personal paths, names or e-mail addresses; noreply commit identity; hygiene checks every commit.
- **Verification**: automation-first; the owner is asked only for identity-bound consent or a run on their own hardware.
- **Delivery**: each phase is one branch and one pull request into a green `main`, released automatically; the release pull request is never held.
- **Process**: one GSD step per session; no chaining; no invented ceremony (`AGENTS.md` rules 8 and 9).

## Key Decisions

| Decision | Rationale | Outcome |
|---|---|---|
| One core, SMS and Game Gear both in v1.0, as machine profiles | Shared Z80, VDP, PSG and mapper; the owner's collection spans both | ✓ Good — both machines play with sound in RetroArch (Phases 3 and 4) |
| Time in VDP-clock ticks; the Z80 drives the bus; devices catch up lazily; per-line VDP rendering with dot-exact interrupt and latch ticks | Every documented edge is an integer on this lattice; cheaper than per-dot on low-power targets | ✓ Good — the PSG and YM2413 catch up on the same lattice with exact tick grids (Phases 2 to 4) |
| The whole Z80 against SingleStepTests on every cycle before any game | Exhaustive MIT vectors exist; siblings that grew CPUs from game traces stalled | ✓ Good — 100-per-file sample per PR, full set nightly (Phase 2) |
| Self-authored WLA-DX fixtures with a RAM mailbox as the primary evidence | Licence-clear, byte-identical builds, oracles that cannot be fooled | ✓ Good — mapper_sega, mapper_codemasters and ports fixtures built in CI (Phase 2) |
| ares (ISC) and MAME's BSD-3 files may be read; Genesis Plus GX, Gearsystem and other GPL or non-commercial sources never | MIT licence protection | — Pending |
| Video is one 256×H RGB444 field with a crop rect; palettes and filters are host helpers | GG-native, SMS-lossless, hash-stable | ✓ Good — independent SMS/GG pixels, completed fields and eight-lane playing hashes (Phase 3) |
| Audio hash on the transition stream; PCM secondary; exact rational samples per frame | Mixer-independent determinism | ✓ Good — transition and PCM hashes equal on the eight hosted lanes, 801 or 802 samples per frame (Phase 4) |
| Every phase ends in an automated release; phase 1 is the ROM-free walking skeleton | Sibling velocity evidence | ✓ Good — v0.1.0 to v0.4.0, one per phase, each by merging a pull request |
| One entrypoint: CMake workflow presets; one required CI check; SHA-pinned actions; measured timeouts | No script to keep in step with CI | ⚠️ Revisit the timeouts — the preset and required check held for four phases, but the windows-arm64 timeout failed one main run |
| Cachegrind instructions per frame as the only per-PR speed gate | Wall-clock tails are noise on shared runners | ⚠️ Revisit — still report-only; arm it against a main baseline that includes sound |
| Proportional gates; real-host e2e ≤ 300 lines; no attempt quotas or ceremonies | Two siblings stalled on their own machinery | ✓ Good — 528 check / 5,298 core lines and 300 hosted helper lines (Phase 3) |
| Pins are public and fetched with `gh api` | A sibling lost six plans to an unpushed pin | — Pending |
| Pre-seeded GSD bootstrap, interactive, one step per session | Owner steering; fastest sibling path | ✓ Good — four phases and an audit in 22 days, 62 plans |
| NMI is not delayed by EI; only /INT waits one instruction after EI | Matches the SingleStepTests vectors | ✓ Good (Phase 2) |
| Game Gear `$00` bit 5 reads 0 on NTSC: `$C0` export, `$80` Japan | SMS-SYSTEM section 8 | ✓ Good — `ports` fixture (Phase 2) |
| The Codemasters `$4000` bank comes from bits 6-0; bit 7 is only the RAM enable | Passing bit 7 picked the wrong bank and raised mapper_range | ✓ Good — `mapper_codemasters` fixture (Phase 2) |
| Writes while `$3E` disables the cartridge are ignored and recorded as unmapped_write | Hardware docs are silent; symmetric with reads, stated in `src/bus.c` | — Pending hardware evidence |
| The Z80 is split into `z80.c`, `z80_cb.c`, `z80_ed.c` and `z80_x3.c` under an 800-line cap per file | Small modules; clang-format 21 and hygiene stay green | ✓ Good (Phase 2) |
| A save state refuses CPU combinations the core cannot reach and ties the beam to the bus stamp | A hostile state must not break the bounded-frame guarantee | ✓ Good — state round-trip and refusal tests (Phase 2) |
| Battery classification is independent of mapper allocation; unsupported physical geometry exposes no guessed span | Metadata membership cannot establish persistence wiring | ✓ Good — exact spans, transactional import and native process continuation (Phase 3) |
| Selected VDP timing remains confidence-qualified until hardware measurements | Passing fixtures prove the model, not new measurements | ✓ Good — documented H/latch/GG assumptions retained at verification (Phase 3) |
| The YM2413 is seeded from MIT emu2413 and proved equal to the unmodified original | The chip is the largest piece of new logic; a lockstep oracle bounds the port risk | ✓ Good — 2,000,000 seeded samples equal against a frozen digest (Phase 4) |
| Band-limited step synthesis with an integer kernel behind one mixer | Alias-free output without floating point | ✓ Good — 85.0 dB alias suppression and 7 integer proofs (Phase 4) |
| Third-party sound ROM conformance is deferred with no lane; owned fixtures judged by frozen hashes | The test ROMs are keyboard-driven exercisers that print nothing | ✓ Good — `psg_sweep` and `fm_sweep` judged by mailbox rows and pinned hashes (Phase 4) |

The full list with sources is `.planning/preparation/DECISIONS.md`.

## Evolution

After each phase transition:
1. Validated requirements move from Active to Validated with the phase number and a ✓ outcome.
2. Key Decisions get their outcome column filled from the phase's verification.
3. New constraints or scope changes are recorded with the date.

After each milestone:
1. The Active list is rewritten from the next milestone's requirements.
2. `.planning/PRODUCT-ROADMAP.md` and `.planning/preparation/PROMPT-COVERAGE.md` are refreshed and snapshotted.
3. Lessons are exported to `.planning/LESSONS.md`.

---
*Last updated: 2026-11-10 after starting milestone v2.0*
