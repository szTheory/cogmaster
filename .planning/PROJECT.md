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

## Current Milestone: v2.1 Fewer failing cartridges

**Goal:** Find out why about a third of the checked cartridges (210 default-profile `fail` rows of 603 judged) fail the
boots check, fix the biggest shared causes, and publish the rows that flip to `pass`.

**Target features:**
- A per-cart diagnostic in the library judge (frames to first non-blank, first fault kind and PC, mapper chosen) that
  prints counts and kinds only, never ROM bytes (SEED-016).
- Fixes in order of group size: check strictness first, then mapper families (`zemina`, `korean_nb`, `korean`), then
  individual faults.
- A re-run of the library judge over the owner's library, with the flipped rows merged by pull request. The agent runs
  it in a shell where the owner passes `COGMASTER_ROM_DIR`, reads counts and catalog titles only, and commits nothing
  from the dumps (owner decision 2026-11-12).
- The CI timeout drift (SEED-015): asan, the nightly bench, bench-wasm32 and the fuzz comment re-measured and edited in
  one workflow change.

## Current State

**v0.13.0 is shipped** (milestone v2.0 "The whole library", phases 5 to 12, releases v0.5.0 to v0.13.0). The public
surface is API 14, behaviour revision 17 and state format 16. The core plays Sega, Codemasters and Korean-board
cartridges (Korean `$A000`, Hicom, Zemina, Nemesis, `korean_nb` and 4 PAK), keeps battery and 93C46 EEPROM saves across
restarts, supports the Sports Pad, Paddle, 3-D glasses and Light Phaser, boots from the user's own BIOS dump on request,
and runs the PAL and Japanese FM profiles. The compatibility ledger holds a row with an honest status for every
Master System and Game Gear entry at the MAME pin. The owner's own-library run is merged (v0.13.0): 396 default-profile
passes and 210 `fail` rows, and the libretro core's `is_experimental` flag is gone. The milestone audit
(`.planning/milestones/v2.0-MILESTONE-AUDIT.md`) found 49 of 49 requirements satisfied, status tech_debt and no
blockers.

## Next Milestone Goals

After v2.1, none of it a gate:

- The pad, Light Phaser and BIOS judges (SEED-010) and the scan-threshold measurement (SEED-009) can ride in the next
  owner sitting.
- The rest of SEED-015 and SEED-007 (ledger rows, review leftovers).
- Later horizons (states and movies, measured accuracy, tools, ports, the browser, SG-1000) stay seeds in
  `.planning/seeds/` and horizons in `.planning/PRODUCT-ROADMAP.md`; each needs the owner's approval at
  `/gsd-new-milestone`.

## Core Value

Master System and Game Gear games behave as they do on the console, identically on every platform, from a small
MIT-licensed C library that any host can embed.

## Requirements

### Validated

- ✓ Release automation from the first phase: library, runner and libretro core built, tested, attested and published on every platform by merging a pull request; one install line puts the core into RetroArch on a Mac — Phase 1
- ✓ The Z80 matches the public SingleStepTests vectors on every cycle before any game runs, with the bus, Sega and Codemasters mappers, ports and the Pause NMI — Phase 2
- ✓ SMS and Game Gear owned fixtures play through the runner and released RetroArch, with two pads, regions, raw battery continuation, identical playing hashes on eight lanes and a protected compatibility ledger; the private-library path is proven with synthetic fixtures — Phase 3
- ✓ Sound: the SN76489 with Game Gear stereo and the YM2413 for the Japanese profile play through the runner and RetroArch, with transition and PCM hashes identical on every platform including the 32-bit big-endian lane and the wasm32 lane — Phase 4
- ✓ A tune-up that cleared the v1.0 CI, perf-gate, review and README debt: timeouts measured from live runs, the perf gate armed, `quality.json` floors and ceilings, Dependabot, the weekly digest and the README's ROM step — v2.0 (Phase 5)
- ✓ The remaining cartridge boards (Korean `$A000`, Hicom, Zemina, Nemesis, `korean_nb`, 4 PAK) detected hash-first with a bounded scan and played with no setting; unsupported boards refuse with exit 77 and a reason; no shipped hash moved — v2.0 (Phase 6)
- ✓ Saves that round-trip: the 93C46 EEPROM, `.srm` import rules measured once, the libretro saves flag, runner `--save-interval` with atomic writes, and hosted RetroArch restarts for both save kinds — v2.0 (Phase 7)
- ✓ Sports Pad, Paddle and 3-D glasses at pin level behind a grown, backward-compatible input struct, with table defaults per dump and replays that match — v2.0 (Phase 8)
- ✓ Light Phaser titles hit where the player aims, sensed from the emitted pixel at the beam position, with no change to any run that has no gun or fires no shot — v2.0 (Phase 9)
- ✓ BIOS boot from the user's own dump in the runner or RetroArch, fast boot staying the default, both firmware legality scans with negative controls, and the `$3E` arbitration checked against the hardware notes — v2.0 (Phase 10)
- ✓ PAL timing proven identical on every lane, FM judged by its sound on owned fixtures, and the library judge able to add PAL and Japanese FM rows through `compat-update.sh` — v2.0 (Phase 11)
- ✓ The whole list in the ledger: one honest row per Master System and Game Gear entry at the pin, a bounded compatibility page, ratchet rules for unsupported rows, and `compat.exit` tying `is_experimental` to the judged-cart floors both ways (satisfied as amended: the flag stays until the owner's own-library run) — v2.0 (Phase 12)
- ✓ The failing default-profile rows have a known cause: `cogmaster-run --diagnose` and `sh scripts/library.sh diagnose` print counts, kinds and catalog titles only, a canary scan with five negative controls proves nothing from a dump leaks, and 205 failing rows (of 209 in the ledger) are grouped by cause in `13-CAUSE-GROUPS.md`; no ledger row moved — v2.1 (Phase 13)

### Active

v2.1 (defined in `.planning/REQUIREMENTS.md`):

- [ ] Fix the largest causes in group-size order and flip the rows they explain to `pass`
- [ ] Re-run the library judge and merge the flipped rows
- [ ] Re-measure and correct the drifted CI job timeouts in one workflow edit

### Out of Scope

- A window, audio device or input device — hosts own presentation; RetroArch is the frontend.
- Shaders, menus, cheats, rewind UI, netplay UI — frontend features; the core exposes what they need.
- Any Sega BIOS or game bytes — copyright; the user's own dumps stay on the user's machine (`ASSET_POLICY.md`).
- GPL, LGPL or non-commercial code — MIT licence (`PROVENANCE.md`).
- A fast or low-accuracy mode that changes timing — exact timing is the product.
- Signed macOS builds — a Developer ID certificate puts a legal name in artifacts.
- Later milestones: states and movies, measured accuracy, tools, ports, the browser and SG-1000 mode are seeds in `.planning/seeds/` and horizons in `.planning/PRODUCT-ROADMAP.md`.

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
- **Shipped v2.0** (8 phases, 94 plans, 49 of 49 requirements; releases v0.5.0 to v0.12.0, one per phase, in four
  days): 9,403 lines of C in `src/`, 671 tests in the `ci` workflow, API 14, behaviour revision 17, state format 16.
  The milestone audit found no gaps and recorded tech debt (`.planning/milestones/v2.0-MILESTONE-AUDIT.md`): timeout
  drift in asan, the nightly bench and bench-wasm32 (W1), the 188-in-1 row's status word (W2), 50 Game Gear rows judged
  as `video=gg` (W3), and review leftovers in seeds SEED-007 and SEED-009 to SEED-015. Three owner hand-offs wait as
  `not_run` and never gate a release: the own-library run (SEED-013), the pad, Light Phaser and BIOS judges (SEED-010)
  and the scan thresholds (SEED-009).
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
| ares (ISC) and MAME's BSD-3 files may be read; Genesis Plus GX, Gearsystem and other GPL or non-commercial sources never | MIT licence protection | ✓ Good — the BSD-3 MAME lists seeded the cartridge catalog and the whole-list ledger; the provenance, notices and hygiene checks stayed green through v2.0 |
| Video is one 256×H RGB444 field with a crop rect; palettes and filters are host helpers | GG-native, SMS-lossless, hash-stable | ✓ Good — independent SMS/GG pixels, completed fields and eight-lane playing hashes (Phase 3) |
| Audio hash on the transition stream; PCM secondary; exact rational samples per frame | Mixer-independent determinism | ✓ Good — transition and PCM hashes equal on the eight hosted lanes, 801 or 802 samples per frame (Phase 4) |
| Every phase ends in an automated release; phase 1 is the ROM-free walking skeleton | Sibling velocity evidence | ✓ Good — v0.1.0 to v0.4.0, one per phase, each by merging a pull request |
| One entrypoint: CMake workflow presets; one required CI check; SHA-pinned actions; measured timeouts | No script to keep in step with CI | ⚠️ Revisit the timeouts — phase 5 re-derived every timeout from live runs and the preset and required check held for eight more phases, but at the audit asan, the nightly bench and bench-wasm32 had drifted below twice their slowest run and CI cannot see it (SEED-015) |
| Cachegrind instructions per frame as the only per-PR speed gate | Wall-clock tails are noise on shared runners | ✓ Good — armed in phase 5 after two zero-spread control runs; a throwaway draft pull request with +4.87 percent failed it, and every later phase reported its delta (0 to -11 basis points) |
| Proportional gates; real-host e2e ≤ 300 lines; no attempt quotas or ceremonies | Two siblings stalled on their own machinery | ✓ Good — 528 check / 5,298 core lines and 300 hosted helper lines (Phase 3) |
| Pins are public and fetched with `gh api` | A sibling lost six plans to an unpushed pin | ✓ Good — the MAME pin was regenerated live at the audit and the catalog, table and counts came out byte-equal |
| Pre-seeded GSD bootstrap, interactive, one step per session | Owner steering; fastest sibling path | ✓ Good — four phases and an audit in 22 days, 62 plans |
| NMI is not delayed by EI; only /INT waits one instruction after EI | Matches the SingleStepTests vectors | ✓ Good (Phase 2) |
| Game Gear `$00` bit 5 reads 0 on NTSC: `$C0` export, `$80` Japan | SMS-SYSTEM section 8 | ✓ Good — `ports` fixture (Phase 2) |
| The Codemasters `$4000` bank comes from bits 6-0; bit 7 is only the RAM enable | Passing bit 7 picked the wrong bank and raised mapper_range | ✓ Good — `mapper_codemasters` fixture (Phase 2) |
| Writes while `$3E` disables the cartridge are ignored and recorded as unmapped_write | Hardware docs are silent; symmetric with reads, stated in `src/bus.c` | — Pending hardware evidence; the BIOS phase tests every `$3E` combination and labels two rows as model choices (Phase 10) |
| The Z80 is split into `z80.c`, `z80_cb.c`, `z80_ed.c` and `z80_x3.c` under an 800-line cap per file | Small modules; clang-format 21 and hygiene stay green | ✓ Good (Phase 2) |
| A save state refuses CPU combinations the core cannot reach and ties the beam to the bus stamp | A hostile state must not break the bounded-frame guarantee | ✓ Good — state round-trip and refusal tests (Phase 2) |
| Battery classification is independent of mapper allocation; unsupported physical geometry exposes no guessed span | Metadata membership cannot establish persistence wiring | ✓ Good — exact spans, transactional import and native process continuation (Phase 3) |
| Selected VDP timing remains confidence-qualified until hardware measurements | Passing fixtures prove the model, not new measurements | ✓ Good — documented H/latch/GG assumptions retained at verification (Phase 3) |
| The YM2413 is seeded from MIT emu2413 and proved equal to the unmodified original | The chip is the largest piece of new logic; a lockstep oracle bounds the port risk | ✓ Good — 2,000,000 seeded samples equal against a frozen digest (Phase 4) |
| Band-limited step synthesis with an integer kernel behind one mixer | Alias-free output without floating point | ✓ Good — 85.0 dB alias suppression and 7 integer proofs (Phase 4) |
| Third-party sound ROM conformance is deferred with no lane; owned fixtures judged by frozen hashes | The test ROMs are keyboard-driven exercisers that print nothing | ✓ Good — `psg_sweep` and `fm_sweep` judged by mailbox rows and pinned hashes (Phase 4) |
| Boards are detected hash-first from binary SHA-1 rows; the instruction-aware scan only decides below a valid `TMR SEGA` header and refuses a tie; unsupported boards exit 77 with a closed reason | A wrong board guess must never turn a Sega game into a Korean one, and the hash table is the shipped truth | ✓ Good — eight-lane equality with no shipped hash moved; the scan thresholds are unmeasured on real dumps (SEED-009, Phase 6) |
| Peripherals come through one pin-level port layer with one TH edge detector, and the input struct grows by copying the smaller caller size | Every device then shares one latch rule, and API 1 to 11 callers stay valid | ✓ Good — a 65,536-pair latch differential, pad-only hashes unchanged on eight lanes, an ASan test of the bounded copy (Phase 8) |
| The Light Phaser senses the emitted pixel at the beam position with no per-title offset table | Hardware senses light, not titles; a table would hide model errors | ✓ Good — `policy.no_title_table`, a fixture judged at ten loop lengths; delay, edge rule and radius are model-derived and unmeasured (Phase 9) |
| BIOS boot is opt-in from the user's own dump; fast boot stays the default; two firmware scans with negative controls keep every BIOS byte out of the tree and the releases | The legal rule is absolute and the default hashes must not move | ✓ Good — a BIOS-shaped image equals the fast-boot hand-off; the real-BIOS residue is an owner hand-off (Phase 10) |
| The ledger has one row per catalog identity with closed reason tokens for `unsupported` and `skipped`, and the ratchet releases such a row only through a catalog change | An honest row beats a missing one, and a status cannot move by editing the ledger alone | ✓ Good — 1,472 rows, `compat.exit` and a bounded page proven in CI (Phase 12) |
| `is_experimental` stays until the owner's own-library run meets the judged-cart floors; `compat.exit` ties the flag to the verdict both ways (owner decision 2026-11-12) | Only the owner's carts and consoles can show real behaviour, and no agent may read them | — Pending the own-library run (SEED-013); the test fails if the flag and the verdict disagree |
| Owner hand-offs are batched, `not_run` and never a gate; a requirement that named one is amended in the phase that builds what CI can prove | Rule 6: the owner verifies nothing by default | ✓ Good — 14 requirements amended with their code, 0 blockers at the audit, every VERIFICATION `human_needed: false` (v2.0) |

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
*Last updated: 2026-11-12 after Phase 13*
