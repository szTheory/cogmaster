---
title: "Preparation index"
summary: "Which preparation file answers which question, and the conventions every file in this directory shares."
read_when: "First, before opening any other file in this directory; again when adding, renaming or retiring one."
updated: 2026-10-18
---

# Preparation index

Reference material gathered before cogmaster's first line of code: Master System and Game Gear hardware facts with
sources, the test and oracle plan, engineering, host and process decisions, and how GSD is run. It is the project's
searchable second brain: each file opens with frontmatter saying when to read it and a Where to look table, cites
tiered, pinned sources, and gives one recommendation per choice. Open the file a question points to; none needs
reading end to end. `BRIEF.md` holds the conventions and the fan-out method every file follows.

## Files
| File | Holds | Read when |
|---|---|---|
| [BRIEF.md](BRIEF.md) | Conventions, source tiers, licence rules for research, the fan-out method | Before writing or revising any file here |
| [ARCHITECTURE.md](ARCHITECTURE.md) | The core's design spine: tick time base and frame boundary, modules and the bus, the public C API, I/O contracts, state, observability, build order, open conflicts | Before designing a core subsystem, the public header, the libretro adapter or the runner |
| [DECISIONS.md](DECISIONS.md) | One row per settled choice (`DEC.NN`) with its reason and supporting section; open questions at the end | Before reopening a choice, or when a phase needs to know what is settled |
| [SMS-SYSTEM.md](SMS-SYSTEM.md) | Clocks and the master tick, the per-line timeline, memory map and paging registers, I/O ports, regions and variants, BIOS and fast boot, reset and Pause, power-on profile | Changing the time base, bus, ports, boot path, region handling or any timing that crosses chips |
| [SMS-VDP.md](SMS-VDP.md) | VDP revisions, registers, mode 4 pipeline, interrupt and counter timing, geometry and palettes, the Game Gear window, TMS legacy modes, test ROMs, the accuracy model | Writing or changing `vdp.c`, the frame format or interrupt timing, or when a picture diverges |
| [SMS-AUDIO.md](SMS-AUDIO.md) | The PSG, Game Gear stereo, the YM2413 and FM detection, integer synthesis, hashes and A/V sync, test ROMs | Building the PSG, the OPLL, the synthesiser, the mixer or audio tests |
| [SMS-CARTRIDGES-AND-PERIPHERALS.md](SMS-CARTRIDGES-AND-PERIPHERALS.md) | The header, every mapper with its library share, controllers at pin level, peripherals, saves and interop, ROM-hacker needs, hostile-input bounds, the milestone table | Changing the loader or probe, a mapper, ports, saves, patch handling or fuzz targets |
| [SMS-ECOSYSTEM-AND-USERS.md](SMS-ECOSYSTEM-AND-USERS.md) | Every SMS/GG emulator with its licence, user complaints mapped to responses, personas by horizon, low-power budgets, library sizes, the slot cogmaster fills | Ranking features or horizons, choosing an oracle, planning a port, answering "why not emulator X" |
| [SMSPOWER-EXTRACTS.md](SMSPOWER-EXTRACTS.md) | Paraphrased SMS Power! and forum facts (tier T4, many measured on hardware), the test-ROM inventory with licences, fixture candidates, the WLA-DX pin and digest | Before relying on a forum claim cited in another file; when admitting a fixture or tool |
| [CONFORMANCE.md](CONFORMANCE.md) | Vectors and test ROMs with dispositions, own fixtures and the mailbox protocol, oracles, the compatibility ledger and ratchet, CI tiers and cost, admission rules | Admitting a fixture, writing diagnostics or the result protocol, building the ledger or a CI tier |
| [HOSTS-AND-INTEGRATION.md](HOSTS-AND-INTEGRATION.md) | libretro contract, RetroAchievements map, RetroArch on Apple Silicon, `cogmaster-run` and Playstead, scripting order and movie formats, WASM | Building the adapter or `.info`, the runner CLI, saves, the RetroArch e2e job or a scripting surface |
| [ENGINEERING.md](ENGINEERING.md) | C rules, presets, tests, fuzzing, CI, release, repository settings, triage, performance ratchet, portability, style, docs, recurring loops, secrets, WASM and low power | Creating or changing the build, tests, workflows, repository settings, the release pipeline or quality gates |
| [GSD-HANDOFF.md](GSD-HANDOFF.md) | The kickoff text, per-phase and milestone routines, settings and why, what stops GSD running on, owner tasks, what was checked | Before the kickoff, before each GSD step, and when a step behaves unexpectedly |
| [PROMPT-COVERAGE.md](PROMPT-COVERAGE.md) | Every owner ask mapped to its home with status and what remains; owner constraints vs preparation's choices | At kickoff review and at every milestone close |
| [PROVENANCE.md](../../PROVENANCE.md) | Which code, tables and facts may enter; the upstream licence audit at each pin; strangler-fig seeds; notices | Before reusing or citing any outside code or data |
| [ASSET_POLICY.md](../../ASSET_POLICY.md) | Which bytes may be committed, built in CI or kept local; BIOS digests; how it is checked | Before adding a ROM, BIOS, save, state, screenshot or fixture |
| [LESSONS.md](../LESSONS.md) | Sibling lessons imported as hypotheses with their local disposition; the export template | At every discuss step, at phase and milestone boundaries, or when the owner asks to export lessons |
| [PRODUCT-ROADMAP.md](../PRODUCT-ROADMAP.md) | Immediate / Near / Mid / Long horizons with evidence, triggers and a decision log | At every milestone step, or when a finding changes priorities |
| [seeds/](../seeds/) | SEED-001 to SEED-008 hold the later horizons' narratives; SEED-007 is the tune-up that opens every later milestone | When a seed's trigger fires, and at every new milestone |

## Where to look
| Question | File and section |
|---|---|
| What is one tick, what happens at each tick of a line, and when do the interrupts and Pause land? | SMS-SYSTEM 1–2; SMS-VDP 4 |
| What answers each address and port, and how do regions and TV types get detected? | SMS-SYSTEM 3–5 |
| What do the BIOSes do, what must fast boot reproduce, which power-on facts are profile fields? | SMS-SYSTEM 6–8; ASSET_POLICY 4 |
| How is mode 4 rendered, when are scroll and registers latched, what do the counters read? | SMS-VDP 2–4 |
| Visible rows, GG window and palette, aspect, CRT vs LCD? | SMS-VDP 5–6 |
| How do the PSG, GG stereo and the YM2413 work, and what seeds the OPLL? | SMS-AUDIO 1–3 |
| How are samples synthesised and hashed, and how is A/V sync kept? | SMS-AUDIO 4–5 |
| What is at each header offset; how are headerless, copier-headered or bad images told apart? | SMS-CARTRIDGES-AND-PERIPHERALS 1 |
| Which mappers exist, how much of the library each covers, which milestone gets it? | SMS-CARTRIDGES-AND-PERIPHERALS 2, 7 |
| How do controllers map to pins, and how do saves and EEPROMs work? | SMS-CARTRIDGES-AND-PERIPHERALS 3, 4 |
| Which parsers face hostile input, and with which bounds? | SMS-CARTRIDGES-AND-PERIPHERALS 6; ENGINEERING 3 |
| Which SMS/GG emulators exist, and what do users dislike or wish for? | SMS-ECOSYSTEM-AND-USERS 1, 2 |
| What do speedrunners, TASers, AI agents and frontends need; can low-power devices keep up? | SMS-ECOSYSTEM-AND-USERS 3, 4 |
| Which test ROMs and vectors may be used, and may this fixture enter the repository? | SMSPOWER-EXTRACTS 6; CONFORMANCE 1, 7; ASSET_POLICY 1–3 |
| Which assembler builds the fixtures and at what pin? | SMSPOWER-EXTRACTS 7; CONFORMANCE 2 |
| Which oracle and diagnostics come first; how do the ledger and the ratchet work? | CONFORMANCE 3–5; ENGINEERING 8 |
| What runs per PR and nightly, and what does it cost? | CONFORMANCE 6; ENGINEERING 4 |
| Which flags, presets, tests and style rules apply? How is a release made or withdrawn? | ENGINEERING 2, 3, 10; ENGINEERING 5 |
| What must the libretro core export, and how does a release reach RetroArch on a Mac? | HOSTS-AND-INTEGRATION 1, 3 |
| What are the runner's options, exit statuses and Playstead mapping; in what order does scripting arrive? | HOSTS-AND-INTEGRATION 4, 5 |
| May this code or table be reused; how do I test with my own dumps? | PROVENANCE 1–5; ASSET_POLICY 4 |
| What is already decided, and how is GSD started and run each phase? | DECISIONS; GSD-HANDOFF 2, 3 |
| Does every owner ask have a home? | PROMPT-COVERAGE 1 |

## Conventions
See `BRIEF.md`: frontmatter, Where to look table, numbered sections, `[PFX.NN]` citations resolving in the file's
Sources table, tiers T1–T4, pins, markers, Rec/Alt/Flip choices, dotted IDs, own words only, no personal data.

## Keeping it current
- A phase that finds one of these files wrong or stale fixes it and its `updated` date in the same pull request.
- When a subsystem lands, its hardware facts graduate into public `docs/hardware/<chip>.md`, which code comments
  cite (ENGINEERING 10, 11), and the preparation file's section shrinks to a pointer there.
- Research for a single phase lives with that phase under `.planning/phases/`; this directory keeps only
  cross-phase reference. A new, renamed or retired file updates both tables here; PROMPT-COVERAGE and
  PRODUCT-ROADMAP are refreshed at each milestone close (GSD-HANDOFF 4).
- SMS Power! answered 503 for many pages on 2026-10-18; facts taken from archived captures are marked in
  SMSPOWER-EXTRACTS and are re-read at the phase 2 and 3 research steps.
