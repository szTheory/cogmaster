---
title: "Decisions already made"
summary: "One line per settled choice for cogmaster, with its reason and the file and section that support it; open questions at the end."
read_when: "Before reopening any choice, and whenever a phase needs to know what is settled. Reopen a decision only with new evidence, and record the change here with its date."
updated: 2026-10-18
---

# Decisions already made

Each row is settled for v1.0 unless a phase finds new evidence; a change keeps the ID, gives the date and links the
evidence. IDs use a dot (`DEC.07`) so they never collide with GSD's `D-NN` phase decisions.

## Product and scope
| ID | Decision | Because | See |
|---|---|---|---|
| DEC.01 | Three deliverables: C library, headless runner `cogmaster-run`, libretro core; no GUI, window, audio-device or input-device code | Owner rule that core repositories own no GUI; RetroArch is the Mac frontend; Playstead launches processes | HOSTS-AND-INTEGRATION 1, 4 |
| DEC.02 | MIT licence | Compatible with every frontend's licence; same as the siblings | PROVENANCE 1 |
| DEC.03 | SG-1000 and SC-3000 come after v1.0 (SEED-006); until then an SG-1000 image is recognised and refused with "unsupported: SG-1000 (planned)" | Owner decision 2026-10-18 (SMS + GG in v1.0) | SEED-006; SMS-VDP 7 |
| DEC.04 | One core, both machines from v1.0: the Game Gear is a machine profile (`gg`) beside `sms1`, `sms2` and the Japanese FM unit; GG differences (160×144 window, 12-bit CRAM, stereo port $06, ports $00–$06, Start, SMS-compat mode) are profile behaviour, never a second emulator | Owner decision 2026-10-18; shared Z80, VDP core, PSG and mapper | SMS-SYSTEM 5; SMS-VDP 6; SMS-AUDIO 2; CARTRIDGES 1 |
| DEC.05 | Exact timing is a feature: hosts are told 1,640,625/27,379 Hz NTSC (Z80 at 315,000,000/88 Hz, 59,736 cycles per frame) and 13,300,856/267,615 Hz PAL; nothing on by default changes speed | Speedrun and TAS timing; TASVideos uses these rates | SMS-SYSTEM 1; HOSTS 1; ECOSYSTEM 3 |
| DEC.06 | Horizon order: players and homebrew developers (v1.0–v2.0), states and movies (v3.0), measured accuracy (v4.0), tools and AI (v5.0), everywhere (v6.0); the browser (SEED-008) is weighed against v2.0 at the first new-milestone step | Personas ranked by reach and dependency; owner priority on WASM | ECOSYSTEM 3; PRODUCT-ROADMAP |
| DEC.07 | v1.0 boards: no mapper, the Sega mapper (with on-cart RAM) and Codemasters; 620/653 SMS and 812/823 GG entries of the CC0 lists (97.0 %) | Library share counted at MAME `1d7a454ed79e` | CARTRIDGES 2, 7 |
| DEC.08 | v1.0 sound: PSG with Game Gear stereo and the YM2413 for the Japanese profile, both in phase 4 | FM titles are a visible feature of the Japanese library; a permissive OPLL exists | SMS-AUDIO 3, 4 |

## Process
| ID | Decision | Because | See |
|---|---|---|---|
| DEC.09 | Pre-seeded GSD bootstrap: the kickoff resumes at roadmap creation; GSD's researcher still runs before every phase | Fastest sibling's path (muhtari: roadmap in 50 minutes, release next day) | GSD-HANDOFF 1–2 |
| DEC.10 | Interactive mode, both chain flags off, one GSD step per session; a hygiene check refuses commits that flip them | Owner: stop after every phase and switch models | GSD-HANDOFF 5–6; AGENTS rule 9 |
| DEC.11 | Every phase ends in an automated release; phase 1 is a ROM-free walking skeleton; no phase whose goal is a study; the release PR is never held | Sibling evidence (LESSONS XFER-001–003, 048) | research/SUMMARY.md; AGENTS rule 10 |
| DEC.12 | Verification is automation-first; settled, never re-proposed; the owner verifies nothing by default; what only a merge can show is confirmed at ship | Owner directive; sibling evidence (XFER-004, 046) | AGENTS rule 6; GSD-HANDOFF 3 |
| DEC.13 | Proportional gates: repository-check code ≤ 10 % of core lines, real-host e2e ≤ 300 lines checked by a test, ≤ 2 review rounds and 1 gap round, review before verification, tooling findings become seeds | Two siblings stalled on their own machinery; one added 10k host-test lines with no emulator change (XFER-006, 044, 064) | AGENTS rule 7; ENGINEERING 3, 11 |
| DEC.14 | No invented ceremony: no attempt quotas, slots, admission or publication rituals; no requirement naming a post-publish fact or a third person; harness failures are triaged, not gated on | XFER-045, 046, 049, 052 | AGENTS rule 8 |
| DEC.15 | Confidence-qualified models: the best-supported timing model with named, cited constants; unmeasured values are labelled; re-plan only with new evidence | Several VDP timings are inferred from emulators, not measured | ARCHITECTURE 13; SMS-VDP 4, 10 |
| DEC.16 | Branch before the first committing GSD step; no commits on `main` once a remote exists; one squash PR per phase, retitled as a Conventional Commit, auto-merged | GSD 1.17.0 branches only at execute; ship titles PRs "Phase N" | AGENTS rule 10; ENGINEERING 6 |
| DEC.17 | `model_profile: inherit`; runtime and model recorded in STATE at every step | The session model is the knob the owner switches; a sibling's stall coincided with a runtime change and could not be measured | GSD-HANDOFF 5; AGENTS rule 9 |
| DEC.18 | Planning milestones are v1.0, v2.0…; packages are pre-1.0 semver: first release 0.1.0, `feat` and breaking changes bump minor | release-please 17.6.1 semantics; meaningful versions for frontends | ENGINEERING 5 |
| DEC.19 | Pins are public: seeds and fixtures are pinned only to commits the GitHub API answers 200 for; sibling code is fetched with `gh api` | muhtari lost six plans to an unpushed sibling SHA (XFER-057) | AGENTS rule 12 |
| DEC.20 | Preparation stages are committed separately and merged by pull request | muhtari's single squashed preparation commit left its sequence unauditable | git history of `prep/pack` |

## Licence and provenance
| ID | Decision | Because | See |
|---|---|---|---|
| DEC.21 | Another project's expression enters only as a seed with its notice, replacement trigger and parity proof; GPL, LGPL, non-commercial and unlicensed source is never opened; reference emulators run only as released binaries | Protects the MIT licence (an AI-assisted MIT emulator had to relicense in 2026) | PROVENANCE 1, 4 |
| DEC.22 | Seeds: muhtari (runner, libretro adapter, state visitor, helpers, tests scaffolding, CI, hygiene, settings, publish); ymfm (BSD-3) or ares (ISC) OPLL for the YM2413; MAME's three CC0 hash lists through an own generator | Fastest correct path; same-owner code needs no notice | PROVENANCE 4 |
| DEC.23 | Z80, VDP, PSG, mappers, ports and boot are own code from documentation and the vectors; ares (ISC) and MAME's BSD-3 files are read for structure and cross-checks, never copied | Keeps every table re-derivable from a cited source | PROVENANCE 2, 4 |
| DEC.24 | No Sega BIOS bytes, ever; fast boot by default; `--boot bios` runs the user's dump recognised by digest | Firmware copyright; the BIOS splash draws the Sega logo | PROVENANCE 6; ASSET_POLICY 1, 4 |
| DEC.25 | Fixtures are pure WLA-DX assembly, MIT, built in CI from WLA-DX v10.7 pinned by tarball SHA-256; devkitSMS-built homebrew is fetched or built at a pin with digests until its crt0 exception is quoted and judged | A ROM carries the licence of every byte in it (XFER-019); WLA-DX links no runtime | CONFORMANCE 2, 7; SMSPOWER-EXTRACTS 7 |
| DEC.26 | MAME's CC0 `hash/sms.xml`, `gamegear.xml` and `sg1000.xml` are the only bundled databases, converted by an own generator to SHA-1 → mapper, region, size facts | CC0; headerless dumps need it; mapper hints never become behaviour hacks | CARTRIDGES 1; PROVENANCE 7 |
| DEC.27 | `LICENSE` and `THIRD-PARTY-NOTICES.md` in every release archive, checked by a packaging test | BSD-3, ISC and MIT notices must travel with binaries | PROVENANCE 5 |

## Architecture
| ID | Decision | Because | See |
|---|---|---|---|
| DEC.28 | One `uint64_t` tick per VDP clock (10,738,635 Hz NTSC): Z80 T-state = 3 ticks, pixel = 2, line = 684 ticks = 228 cycles, frame = 262 or 313 lines; a Z80-driven bus with one deadline dispatcher; devices catch up lazily to the access stamp | Every documented edge is an integer on this lattice; CPU and VDP phase never drift | SMS-SYSTEM 1, C1, C3; ARCHITECTURE 2 |
| DEC.29 | Default VDP is the 315-5246 (SMS2) model with SMS1 and GG profiles; never the 315-5313 Genesis mode | The SMS2 chip is the common hardware; SMS1 quirks (zoom slots, mask, Graphics II masking) are profile fields | SMS-VDP 1, 8 |
| DEC.30 | The VDP renders per line with catch-up to the current dot before CRAM, register 0/1/7, VRAM and SAT writes; interrupt and latch positions are dot-exact (frame /INT at tick 607, line /INT at 608, latch at 600, V counter at 604); a per-dot model arrives only if a measured case needs it | Reproduces the logic-analyser measurements; per-dot stepping costs more on low-power targets | SMS-VDP 3, 4, 9; ARCHITECTURE 2 |
| DEC.31 | A frame runs from frame-interrupt tick to frame-interrupt tick; input is applied at that boundary; `run_frame` stops at the first instruction boundary at or after frame end and charges the overshoot (≤ 69 ticks) to the next frame | Lowest input latency; bounded calls | ARCHITECTURE 2, 5 |
| DEC.32 | Public API `cogmaster_*`: opaque instance, size-tagged fixed-width structs, append-only statuses, caller-owned buffers, validate then apply, an error changes no state, separate ABI/behaviour/state/battery versions | Sibling API conventions that held over three milestones | ARCHITECTURE 4 |
| DEC.33 | Video is one internal 256×H field of `uint16` RGB444 per dot with a crop rect (GG 48, 24, 160, 144); SMS 6-bit colours expand `c | c << 2`; the hash covers the crop; palettes, aspect and LCD filters are host helpers outside the hash | GG-native and SMS-lossless; CRAM can change mid-frame so a palette-index plane is not a stable identity | SMS-VDP 5, 6, 8; ARCHITECTURE 5 |
| DEC.34 | Audio is int16, one channel on the SMS and two on the Game Gear, at an exact rational samples-per-frame; the primary hash covers the transition stream (tick, source, level), PCM is secondary; synthesis cost is bounded per sample | Mixer-independent determinism; hostile register hammering | SMS-AUDIO 4, 5; ARCHITECTURE 5 |
| DEC.35 | PSG model: 16-bit LFSR with taps 0 and 3, reset to $8000 on every noise-register byte, period 0 behaves as 1, 2 dB volume steps from Maxim's table; GG port $06 is not decoded in SMS mode (open conflict, default recorded) | Maxim's measurements; MAME and ares agree | SMS-AUDIO 1, 2; ARCHITECTURE 13 |
| DEC.36 | YM2413 at Z80/72 Hz with port $F2 selecting PSG/FM/none/both; FM:PSG mix 4096:1434 (Q12) until a hardware recording settles it; export consoles ignore FM writes | Yamaha manual; MAME bug 5849; no schematic found | SMS-AUDIO 3; ARCHITECTURE 13 |
| DEC.37 | Mappers dispatch through a `switch` on the board kind, never a const pointer table; the probe order is strip copier header → SHA-1 table → header → size rules → opcode heuristics → Sega mapper; images are padded to a 16 KB multiple and banks wrap modulo pages | Under PIC a pointer table counts as mutable data (XFER-071); headerless dumps are common | CARTRIDGES 1, 2; ENGINEERING 2 |
| DEC.38 | Controllers are modelled at pin level ($DC/$DD/$3F with TH/TR/TL); a lag frame is one with no read of $DC, $DD, $C0, $C1 or GG $00; Pause is an NMI edge aligned to the start of the last line | Light Phaser, Sports Pad and region detection live at pin level; TASVideos' lag definition | CARTRIDGES 3; SMS-SYSTEM C8 |
| DEC.39 | Every run call returns within a bounded number of ticks, including HALT/DI loops and DMA-free VDP hammering; VRAM access-slot corruption is never emulated in v1.0, only counted and recorded as a fault | Hostile ROMs; frontends must never hang; no slot schedule is published | ARCHITECTURE 2, 13; SMS-VDP 4 |
| DEC.40 | Per-frame and per-line counters are always kept; the first fault (PC, opcode, address, width, device, frame, line, dot) is captured from phase 1; hooks use a dual-compiled traced loop chosen only when installed (v5.0) | Best-in-class observability at zero cost; a sibling never captured its last fault's address (XFER-055) | ARCHITECTURE 8 |
| DEC.41 | Fixed-size state written by one four-pass visitor from phase 1 (format freezes in v3.0), validated before applying, byte-wise little-endian, ticks capped; battery span exported raw with a generation counter | Sibling design; deterministic run-ahead and netplay | ARCHITECTURE 7 |
| DEC.42 | Profile fields: machine, VDP chip, region, TV, FM, RAM fill (zero default), open-bus rule (last data-bus byte on SMS1, $FF on SMS2/GG), paging reset 0/0/1/2, port and PSG power-on values | Hardware varies; tests need fixed values | SMS-SYSTEM 8; ARCHITECTURE 6 |
| DEC.43 | Fast boot sets the documented hand-off: port $3E = $AB (SMS) / $A8 (GG), RAM $C000 = $AB, $3F = $FF, SP = $DFF0, paging 0/0/1/2, and writes no BIOS code into RAM | No Sega bytes; what the BIOS leaves behind is documented | SMS-SYSTEM 6, C7; PROVENANCE 6 |
| DEC.44 | Open-conflict defaults until measured: vertical scroll latched per frame; SMS1 zoom applies to the first four sprites of a line (slot i wide iff i + 4 < count rule from the wiki); overflow and collision gated to active lines | Documented positions with the local test that settles each | ARCHITECTURE 13; SMS-VDP 10 |

## Testing and conformance
| ID | Decision | Because | See |
|---|---|---|---|
| DEC.45 | SingleStepTests `z80/v1`: the first 100 tests of each of the 1,604 files committed in a compact binary form; the full set nightly at `ebe1875d48f3`; every opcode matches on final state and on every cycle, including MEMPTR, Q and R | Exhaustive per-cycle CPU evidence at PR cost (XFER-005) | CONFORMANCE 1 |
| DEC.46 | ZEXALL/ZEXDOC run nightly as fetched fixtures; FluBBa's and sverx's suites run local and nightly as binaries with results read through `peek`; JoppyFurr's MIT PSG and FM test ROMs may be committed under the manifest | Licences: GPL-2, none, MIT respectively | CONFORMANCE 1; SMSPOWER-EXTRACTS 6 |
| DEC.47 | Own fixture set in v1.0: `boot_to_play`, `vdp_lines`, `mapper_sega`, `mapper_codemasters`, `battery`, `psg_sweep`, `ports`, with GG variants; each has a counter only a new input changes and reports numbers through a RAM mailbox, never verdicts | The oracle cannot be fooled (XFER-025); the same bytes run on hardware | CONFORMANCE 2, 4 |
| DEC.48 | Oracles: released binaries (ares, Genesis Plus GX in RetroArch, Emulicious, MAME) nightly as locators, never gates; MiSTer RTL facts as the timing referee in v4.0; real hardware arbitrates | Licence rules; strongest references | CONFORMANCE 3; ECOSYSTEM 5 |
| DEC.49 | Compatibility ledger CSV → generated `docs/compatibility.md`; passes only ratchet up; the owner's dumps run only in a local `library` preset never registered in `ci`; the only identity that leaves the machine is the SHA-1 already public in the CC0 list | Measurable compatibility without committing ROMs or self-skipping tests | CONFORMANCE 5 |
| DEC.50 | Exact executed-test inventory, no self-skip, negative controls asserting the specific failure, acceptance on exit status and tolerant regexes, one registration file per subsystem | Sibling test-gate failures (XFER-013, 058) | ENGINEERING 3 |
| DEC.51 | Property tests use a small in-repo seeded harness; fuzz targets `fuzz.loader`, `fuzz.state`, `fuzz.save`, `fuzz.run`, report-only until green for ten runs | `theft` needs `fork`; a harness bug blocked a sibling's release (XFER-049, 073) | ENGINEERING 3 |

## Engineering and delivery
| ID | Decision | Because | See |
|---|---|---|---|
| DEC.52 | C17 without extensions; warnings fatal only in presets; integer-only, no globals, one symbol namespace, header valid C and C++, no host clock; each enforced by a test | Embeddable everywhere | ENGINEERING 2 |
| DEC.53 | Six platforms on every PR plus a 32-bit big-endian PowerPC lane under qemu and a wasm32-wasi hash lane (report-only until ten green runs); armhf nightly; console compile lanes from v6.0 | Identical output on every platform is a product claim | ENGINEERING 4, 9, 14 |
| DEC.54 | One required check `CI required`; `push` runs only on `main`; SHA-pinned actions; timeouts twice the slowest measured run with its run ID; no cache until one is measured to pay; CI shell scripts also run under dash | Sibling CI evidence (XFER-012, 021, 042, 067) | ENGINEERING 4 |
| DEC.55 | Ubuntu 26.04 runners for Linux jobs; clang-format 21 pinned; `CMAKE_OSX_SYSROOT` set in the hygiene preset | Current images; one formatter version; CMake 4 leaves the sysroot empty (XFER-060) | ENGINEERING 1, 2 |
| DEC.56 | Releases: release-please `simple` with a GitHub App token, exactly 18 archives, `SHA256SUMS`, verified attestations, version-free asset names, `version.txt` the only machine-edited version, drafts addressed by ID, a rehearsal on the pull request that re-runs when release inputs change and is part of `CI required` | Sibling release footguns (XFER-008, 009, 027, 039, 064) | ENGINEERING 5 |
| DEC.57 | Repository settings as code (squash title and body from the PR), applied at preparation with `--no-ruleset`; the committed ruleset is applied in phase 1 once CI reports | GSD's ship step pushes a CI-skip commit; a required check that never reports blocks every merge | ENGINEERING 6; `scripts/apply-repo-settings.sh` |
| DEC.58 | Cachegrind instructions per frame is the only per-PR speed gate (+1 %), armed in phase 1–2 after two zero-spread control runs; wall-clock percentiles to p99.999 over ≥ 300,000 frames are nightly trend data; zero allocations after load is a test | Shared runners make wall-clock tails noise; a sibling found its regression at milestone end (XFER-051) | ENGINEERING 8 |
| DEC.59 | One `quality.json` ratchet with one check, built in the v2.0 tune-up; per-metric gates work from v1.0 | Baselines only ratchet up | ENGINEERING 8; SEED-007 |
| DEC.60 | Docs budgets: user-facing README ≤ 12 KB; maintainer docs in `docs/`; public header ≤ 24 KB; hardware facts graduate to `docs/hardware/` as subsystems land | A sibling README reached 114 KB | ENGINEERING 11 |
| DEC.61 | Each milestone opens with a tune-up phase (CI efficiency, weakest quality dimension, ratchets, freshness, roadmap and coverage refresh, lesson export) | The owner's recurring audits, automated | SEED-007; ENGINEERING 12 |

## Hosts and integration
| ID | Decision | Because | See |
|---|---|---|---|
| DEC.62 | libretro: Genesis Plus GX-compatible RetroPad layout (B = 1, A = 2, Start = Pause/Start), its BIOS file names (`bios_E.sms`, `bios_U.sms`, `bios_J.sms`, `bios.gg`, all optional), its option keys where the meaning matches, rcheevos memory descriptors (8 KB system RAM plus the 32 KB cartridge-RAM block) tested with rcheevos' own resolver; `.info` claims each tied to a test | Remaps, BIOS folders and achievements carry over for users of the dominant core | HOSTS 1, 2 |
| DEC.63 | Real-RetroArch end-to-end stays two hosted cases (screenshot compare in v1.0, save round trip in v2.0), ≤ 300 lines total, a CMake `-P` driver, no stateful shell harness; semantics are proven in-process | A sibling's RetroArch e2e grew past 10k lines (XFER-044, 067) | HOSTS 3; AGENTS rule 7 |
| DEC.64 | Runner exit statuses 0, 1, 2, 3, 4, 5, 77 in one table; `--probe` prints header fields, region, size, SHA-1, SHA-256, CRC-32 and the RetroAchievements MD5; SIGTERM finishes the frame, flushes the save and exits 0; Playstead's `sms` and `gg` system IDs and RetroArch-with-core launch are Playstead-side owner decisions; the continuation protocol lands in v3.0 | Playstead's adapter contract; headered and headerless dumps both exist | HOSTS 4 |
| DEC.65 | Scripting order: inspection (v1.0) → hooks, Python bindings and a vectorised Gymnasium environment (v5.0) → Lua on demand; an own text movie format with BK2 and MMV import later | YAGNI order by demand | HOSTS 5 |
| DEC.66 | ROM patching stays in the host (RetroArch soft-patching, so `need_fullpath` is false); `cogmaster-run --patch` (IPS/BPS, fuzzed) and WLA-DX `.sym` loading come with the v5.0 tools | Core stays minimal | CARTRIDGES 5 |
| DEC.67 | Saves: export exactly the on-cart RAM (raw) or the 93C46 as 64 little-endian words with a generation counter; pad on import, never on export; `.srm` sizes of other emulators are measured from their binaries in the v2.0 save phase | Interop with other emulators without guessing | CARTRIDGES 4 |

## Open questions (settled by tests or measurements in the phase named)
- Frame and line interrupt tick (607/608 from the logic analyser vs FluBBa's ~654 estimate vs ares' tick 0 of line 193): phase 3 `vdp_lines` fixture against released binaries; hardware in v4.0.
- Vertical scroll latch per frame vs per line; SMS1 zoom slot rule; overflow/collision gating outside active lines: phase 3 fixtures; hardware in v4.0.
- SMS1 open-bus byte per instruction; Japanese/Mark III port decode (full vs partial); $F2 readback on export units: phase 2 `ports` fixture, v4.0 hardware.
- VRAM access-slot corruption (26 vs 29 cycles) and whether the twin-ASIC GG differs: counted in v1.0, modelled only with a measurement (v4.0).
- GG port $06 in SMS mode; GG stereo bit polarity (wiki vs smstech): phase 4 on the owner's Game Gear.
- FM:PSG mix ratio and SMS2 volume clipping: phase 4 recording of the Japanese BIOS tune on hardware.
- Light Phaser H-latch edge; Sports Pad timeout; Janggun flag bit; `.srm` sizes and EEPROM word order in GPGX and Gearsystem; WLA-DX `.sym` layout: v2.0 and v5.0 phases as named in CARTRIDGES 8.
- Whether GSD 1.17.0 still re-routes a stale `gaps_found` without re-verifying (XFER-053): checked at the first `gaps_found`.
