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

## Core Value

Master System and Game Gear games behave as they do on the console, identically on every platform, from a small
MIT-licensed C library that any host can embed.

## Requirements

### Validated

(None yet; the first phase validates the delivery path.)

### Active

- [ ] Release automation from the first phase: library, runner and libretro core built, tested, attested and published on every platform by merging a pull request; one install line puts the core into RetroArch on a Mac.
- [ ] The Z80 matches the public SingleStepTests vectors on every cycle before any game runs, with the bus, Sega and Codemasters mappers, ports and the Pause NMI.
- [ ] Master System and Game Gear games play in RetroArch with two pads, regions, battery saves, self-authored fixtures that prove play, the owner's carts through a local preset, and a compatibility ledger that only ratchets up.
- [ ] Sound: the PSG with Game Gear stereo and the YM2413 for the Japanese profile, with frame and audio hashes identical on every platform including a 32-bit big-endian lane and a wasm32 lane.

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
| One core, SMS and Game Gear both in v1.0, as machine profiles | Shared Z80, VDP, PSG and mapper; the owner's collection spans both | — Pending |
| Time in VDP-clock ticks; the Z80 drives the bus; devices catch up lazily; per-line VDP rendering with dot-exact interrupt and latch ticks | Every documented edge is an integer on this lattice; cheaper than per-dot on low-power targets | — Pending |
| The whole Z80 against SingleStepTests on every cycle before any game | Exhaustive MIT vectors exist; siblings that grew CPUs from game traces stalled | — Pending |
| Self-authored WLA-DX fixtures with a RAM mailbox as the primary evidence | Licence-clear, byte-identical builds, oracles that cannot be fooled | — Pending |
| ares (ISC) and MAME's BSD-3 files may be read; Genesis Plus GX, Gearsystem and other GPL or non-commercial sources never | MIT licence protection | — Pending |
| Video is one 256×H RGB444 field with a crop rect; palettes and filters are host helpers | GG-native, SMS-lossless, hash-stable | — Pending |
| Audio hash on the transition stream; PCM secondary; exact rational samples per frame | Mixer-independent determinism | — Pending |
| Every phase ends in an automated release; phase 1 is the ROM-free walking skeleton | Sibling velocity evidence | — Pending |
| One entrypoint: CMake workflow presets; one required CI check; SHA-pinned actions; measured timeouts | No script to keep in step with CI | — Pending |
| Cachegrind instructions per frame as the only per-PR speed gate | Wall-clock tails are noise on shared runners | — Pending |
| Proportional gates; real-host e2e ≤ 300 lines; no attempt quotas or ceremonies | Two siblings stalled on their own machinery | — Pending |
| Pins are public and fetched with `gh api` | A sibling lost six plans to an unpushed pin | — Pending |
| Pre-seeded GSD bootstrap, interactive, one step per session | Owner steering; fastest sibling path | — Pending |

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
*Last updated: 2026-10-18 during preparation*
