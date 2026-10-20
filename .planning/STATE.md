---
gsd_state_version: "1.0"
milestone: v1.0
milestone_name: SMS and Game Gear games play with sound in RetroArch
current_phase: 03
current_phase_name: Games play in RetroArch
status: ready_for_code_review
stopped_at: Completed 03-18-PLAN.md; ready for code review; runtime Codex; model GPT-6
last_updated: "2026-10-20T23:04:41.252Z"
last_activity: 2026-10-20
last_activity_desc: Completed plan 03-18; ready for code review; runtime Codex; model GPT-6
state_head: dd40d2f54987a149917a4f281f9d6edab9280fe5
progress:
  total_phases: 4
  completed_phases: 2
  total_plans: 47
  completed_plans: 47
  percent: 50
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-10-20)

**Core value:** Master System and Game Gear games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.
**Current focus:** Phase 03 — Games play in RetroArch

## Current Position

Phase: 03 (Games play in RetroArch) — EXECUTION COMPLETE; REVIEW PENDING
Plan: 18 of 18
Status: Ready for code review before formal verification; phase 03 remains pending
Last activity: 2026-10-20 — Completed plan 03-18; runtime Codex; model GPT-6

Next command: `$gsd-code-review 03`. Do not advance phase 04 before the separate review and verification steps.

Progress: [█████░░░░░] 50%

## Performance Metrics

**Velocity:**
- Total plans completed: 47
- Average duration: — min
- Total execution time: 0.0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01 | 12 | - | - |
| 02 | 17 | - | - |
| 03 | 18 | - | - |

**Recent Trend:**
- Last 5 plans: —
- Trend: —

*Updated after each plan completion*
**Per-Plan Metrics:**

| Plan | Duration | Tasks | Files |
|------|----------|-------|-------|
| Phase 01 P01 | 12min | 3 tasks | 25 files |
| Phase 01 P02 | 13min | 2 tasks | 13 files |
| Phase 01 P03 | 5min | 1 tasks | 15 files |
| Phase 01 P04 | 8min | 2 tasks | 22 files |
| Phase 01 P05 | 6min | 3 tasks | 13 files |
| Phase 01 P06 | 6min | 2 tasks | 15 files |
| Phase 01 P07 | 4min | 3 tasks | 20 files |
| Phase 01 P08 | 45 min | 3 tasks | 25 files |
| Phase 01 P09 | 12min | 2 tasks | 12 files |
| Phase 01 P10 | 25min | 2 tasks | 9 files |
| Phase 01 P11 | 13min | 2 tasks | 1 files |
| Phase 01 P12 | 20min | 2 tasks | 11 files |
| Phase 02 P01 | 10min | 2 tasks | 18 files |
| Phase 02 P02 | 12min | 2 tasks | 13 files |
| Phase 02 P03 | 5min | 2 tasks | 6 files |
| Phase 02 P04 | 10min | 3 tasks | 7 files |
| Phase 02 P05 | 25min | 2 tasks | 9 files |
| Phase 02 P06 | 9min | 3 tasks | 28 files |
| Phase 02 P07 | 5min | 2 tasks | 16 files |
| Phase 02 P08 | 9min | 2 tasks | 16 files |
| Phase 02 P09 | 40min | 3 tasks | 21 files |
| Phase 02 P10 | 40min | 3 tasks | 21 files |
| Phase 02 P11 | 9min | 3 tasks | 20 files |
| Phase 02 P12 | 36min | 2 tasks | 19 files |
| Phase 02 P13 | 50min | 3 tasks | 27 files |
| Phase 02 P14 | 25min | 2 tasks | 11 files |
| Phase 02 P15 | 7 min | 2 tasks | 13 files |
| Phase 02 P16 | 23 min | 2 tasks | 15 files |
| Phase 02 P17 | 14min | 2 tasks | 20 files |
| Phase 03 P01 | 16min | 3 tasks | 42 files |
| Phase 03 P02 | 18min | 2 tasks | 37 files |
| Phase 03 P03 | 24min | 2 tasks | 29 files |
| Phase 03 P04 | 16min | 2 tasks | 22 files |
| Phase 03 P05 | 9min | 2 tasks | 15 files |
| Phase 03 P06 | 17min | 2 tasks | 16 files |
| Phase 03 P07 | 14min | 2 tasks | 24 files |
| Phase 03 P08 | 9min | 2 tasks | 34 files |
| Phase 03 P09 | 13min | 2 tasks | 19 files |
| Phase 03 P10 | 10min | 2 tasks | 23 files |
| Phase 03 P11 | 14min | 2 tasks | 36 files |
| Phase 03 P12 | 12min | 2 tasks | 21 files |
| Phase 03 P13 | 10min | 2 tasks | 16 files |
| Phase 03 P14 | 5min | 2 tasks | 11 files |
| Phase 03 P15 | 14min | 2 tasks | 30 files |
| Phase 03 P16 | 7min | 2 tasks | 16 files |
| Phase 03 P17 | 12min | 2 tasks | 24 files |
| Phase 03 P18 | 31min | 2 tasks | 27 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table; the full list with sources is
`.planning/preparation/DECISIONS.md`. Recent decisions affecting current work:

- [Roadmap]: Four phases in the order delivery → CPU → play → sound; every phase ends in an automated release by merging its pull request.
- [Roadmap]: The whole Z80 is proven against SingleStepTests on every cycle (Phase 2) before any game runs (Phase 3).
- [Roadmap]: Self-authored WLA-DX fixtures with a RAM mailbox are the primary evidence; the OPLL seed is chosen at Phase 4 planning (DEC.22).
- [Phase 01]: Status values settled: OK 0, ERR_ARGUMENT 1, ERR_STRUCT_SIZE 2, ERR_NO_MEMORY 3, ERR_BUFFER_TOO_SMALL 4, ERR_NO_CONTENT 5, ERR_PROFILE_REJECTED 6
- [Phase 01]: get_info before a successful load returns ERR_NO_CONTENT
- [Phase 01]: get_frame refuses pitch_bytes above 65536 with ERR_ARGUMENT
- [Phase 01]: Pinned card hashes equal the planning oracle (SMS e14d77ce, GG 7404ed4a); wrong-pin control runs through expect_fail.cmake
- [Phase 01]: State visitor header carries frame, ticks and the packed profile once; a state loads only into an instance of the same machine
- [Phase 01]: Internal state load refuses everything with ERR_ARGUMENT (ERR_NO_CONTENT before a load) until the public state API brings state statuses
- [Phase 01]: cogmaster_get_fault is OK with present 0 before a load; reset POWER also zeroes the counters, SOFT keeps them
- [Phase 01]: PNG encoder fills a caller buffer (png_size, png_write); runner writes each PNG with one fwrite and removes a partial file
- [Phase 01]: Host colour helpers build as the cogmaster-host static library inside the runner block, outside the hash
- [Phase 01]: The bench report has exactly nine lines (frames, p50_us to p99.999_us, max_us, max_rss_kib); --bench caps N at 1073741823 and exits 2 above it
- [Phase 01]: Signal handling is proven by a second runner build compiled with COGMASTER_RUN_RAISE_AT; the installed runner has no test hook
- [Phase 01]: perf_compare.cmake takes MODE=gate|report; report mode prints the verdict and exits 0
- [Phase 01]: libretro core: one instance from retro_init to retro_deinit; av_info and audio length from cogmaster_get_info (48000 x fps_den / fps_num per frame, carried as a remainder)
- [Phase 01]: cogmaster-host builds for the runner or the libretro core, PIC and hidden
- [Phase 01]: Test-only modules cogmaster_libretro_rgb565 and cogmaster_libretro_gg cover RGB565 with single-sample audio and the GG geometry; .info claims only supports_no_game, supported_extensions empty
- [Phase 01]: API version rule kept from the seed: every header name must appear in tests/api/vK.c with K <= COGMASTER_API_VERSION (no git tag read)
- [Phase 01]: docs.api_names drops header names ending in _ (comment wildcards) and lets the guide cite names from cmake/version.h.in and CMakeLists.txt via -DOTHER
- [Phase 01]: Install rules and packaging register only at top level; an embedding project installs nothing of cogmaster's
- [Phase 01]: install_line.cmake rewrites a file:// SHA256SUMS only when it names the libretro zip alone, so a release's full list is never touched
- [Phase 01]: Hygiene preset finds clang-format and clang-tidy 21 from a fixed HINTS list with REQUIRED and checks both majors
- [Phase 01]: release_inputs is forced true with heavy only for untrusted input (no paths, bad or over-3000 count)
- [Phase 01]: check-title.sh and classify-changes.sh run in the C locale
- [Phase 01]: 01-09: wasm32 stays out of CI required's needs (REPORT_ONLY=wasm32); its Linux x64 comparison only reports
- [Phase 01]: 01-09: perf mode is the workflow env COGMASTER_PERF_MODE=report; arming changes it to gate in one ci: pull request
- [Phase 01]: Rehearsal and release publish both use the App token as GH_TOKEN for every publish.sh step; actions/attest signs with the job's OIDC token
- [Phase 01]: publish.sh verify honours SIGNER_WORKFLOW only for rehearsal-* tags; release tags always verify as release.yml
- [Phase 01]: A fork pull request changing a release input skips the rehearsal and fails CI required; rehearse from a same-repository branch
- [Phase 01]: release.yml keeps the seed's concurrency queue: max though actionlint 1.7.12 rejects it (green on muhtari run 38983378877)
- [Phase 01]: MSVC's own constant names (??_C@_, __xmm@) and the x64 vectorizer's __isa_available pair are exempt from the link-surface checks on Windows only; __real@ stays forbidden
- [Phase 01]: Hosted CI green on draft PR #3 (run 39335167905); the merge ref is accepted as the rehearsal's target_commitish
- [Phase 01]: 01-12: Timeouts are twice the slowest successful run here; the 5-minute floor also covers asan and perf (apt) and cleanup (API); all provisional until ten samples
- [Phase 01]: 01-12: be32 timeout scale 8 and wasm32 scale 5, measured against the Linux x64 leg
- [Phase 01]: 01-12: workflow.test_gate_timeout 4 s, twice the slowest clean local ci workflow run (1.61 s)
- [Phase 02]: Vector sample: access form with a 25-bit final-state change mask, 13,017,674 B; magic CGM-Z80V v1, per-file u16 index
- [Phase 02]: Z80 bus primitives call the seam at T3 (M1, memory read and write) and T4 (I/O); the harness subtracts a per-kind offset
- [Phase 02]: z80 sources enter the vector harness as an OBJECT library with the core's options, never through the cogmaster archive
- [Phase 02]: Vector negative control re-encodes one test in a build-tree copy (an unchanged final field lives only in the initial state)
- [Phase 02]: Allow-list lines quote the upstream file name and count as unmatched only when their file ran
- [Phase 02]: Z80 accumulator, INC/DEC and flag helpers are static inline in z80_cycle.h for reuse by the CB and ED pages; stack, pair and condition helpers stay static in z80.c
- [Phase 02]: A halted step runs one M1 at PC and steps PC back so R counts and the step stays bounded
- [Phase 02]: Each DD/FD byte is its own step: step() returns after the prefix fetch with idx set and clears idx after any other opcode
- [Phase 02]: Index remapping is centralised in hx/r8/mem_hl/rp_hi; EX DE,HL and EXX use the true HL pair
- [Phase 02]: NMI is not delayed by EI; only /INT waits one instruction after EI; both wait for a pending prefix
- [Phase 02]: Block instructions are one iteration per step; ED holes (176) are 8-T NOPs reaching the ed_undefined seam
- [Phase 02]: Beam starts at the frame origin (line 192, tick 607, ticks 0) after load and reset; origin constants live in bus.c apart from any interrupt tick
- [Phase 02]: bus.at (the tick line/t describe) is saved state; a state needs at <= ticks <= at + 69 and deadline = (frame + 1) x 179,208
- [Phase 02]: API version checks in v1.c, header and consumer tests accept any version from 1 up
- [Phase 02]: A first-size cogmaster_media tag reads region and tv as AUTO (region sits in the 64-bit tail padding)
- [Phase 02]: State header profile word is machine, region, tv and a zero byte, each fixed to the instance
- [Phase 02]: The V counter is computed from the beam at each read; no stored 604 step
- [Phase 02]: The VDP access tick is m->bus.vdp_at, keeping the VDP struct free of 64-bit fields
- [Phase 02]: The Game Gear profile raises no Pause NMI
- [Phase 02]: VDP port decode: A7/A6/A0 mirrors on the export SMS, full decode on the Japanese SMS and Game Gear
- [Phase 02]: 02-09: The opcode scan refuses only when stores to another board's registers outnumber stores to the Sega registers $FFFC-$FFFF; Codemasters $8000 stores count for neither
- [Phase 02]: 02-09: Bank numbers are reduced modulo the page count at write time; the $FFFC shift is added before the reduction
- [Phase 02]: 02-09: Cartridge RAM lives after the padded ROM in one allocation (32 KiB Sega, 8 KiB Codemasters) and is state; format version stays 2
- [Phase 02]: 02-09: cogmaster_cart_info first size is 52; machine_guess and region_guess are 0 when no header names them
- [Phase 02]: 02-09: Load refuses any 8 KiB body loaded as a Game Gear with ERR_UNSUPPORTED_BOARD (8)
- [Phase 02]: Game Gear $00 bit 5 reads 0 on NTSC ($C0 export, $80 Japan), per SMS-SYSTEM section 8
- [Phase 02]: The step stores the instruction address and first byte after DD/FD in the bus struct for the fault record; both are saved in the state
- [Phase 02]: Game Gear: $3E not decoded (write is unmapped_write), $3F latches, $DD reads $FF
- [Phase 02]: unmapped_write covers $00-$3F and $80-$BF on full-decode consoles; sound-chip and I/O-chip writes never fault
- [Phase 02]: Probe header fields print none without a ROM header; machine_guess and region_guess report what auto resolves to
- [Phase 02]: The probe exits 77 for an unsupported size or board after printing every key; exit 77 is named refused
- [Phase 02]: The first-fault trace prints its line before any hash or bench output
- [Phase 02]: COGMASTER_FIXTURES is declared in CMakeLists.txt so bench's OFF is a used variable
- [Phase 02]: hashes.cmake globs FIXTURES/*.sms sorted, so new fixtures join hashes.txt without edits
- [Phase 02]: MSVC x64 __ImageBase allowed in abi.undefined: linker-defined jump-table base
- [Phase 02]: cogmaster_peek returns a value above 255 (256) for no answer; undriven addresses peek $FF without counting or faulting
- [Phase 02]: Mailbox judge order: magic, then status (hang), then CRC; CSV test column is '<id hex>:<name>'
- [Phase 02]: Fixtures finish no earlier than their second frame (wait for the VDP frame flag) so a one-frame run is a hang
- [Phase 02]: mapper_codemasters has no TMR SEGA header; its mailbox test passes --machine sms through an optional MACHINE variable
- [Phase 02]: A Codemasters slot-0 bank loop runs from a position-independent copy in work RAM at $C200
- [Phase 02]: ports tv_loop rows span the documented approximate count to the frame-length count plus one loop: NTSC 1,923-1,928, PAL 2,287-2,303
- [Phase 02]: A Game Gear fixture image is the SMS source assembled with FIXTURE_GG (region code 6), judged against the same rom's rows under profile gg
- [Phase 02]: Fuzz loader entry loads on the probe's machine and region guess and aborts when probe and load disagree on accepting an image
- [Phase 02]: libFuzzer nightly job stays continue-on-error until ten green runs; crash inputs uploaded as fuzz-artifacts
- [Phase 02]: vectors-full lane: COGMASTER_VECTORS_FULL selects its own inventory; the build converts the fetched archive and registers seven full shards plus vectors.sample_equal; nightly job vectors joins report
- [Phase 02]: Every phase-2 test takes the 10 s TIMEOUT floor (slowest Linux x64 0.28 s); libFuzzer runs 123 s; be32 and wasm32 scales stay 8 and 5 since every test fits with wide margin
- [Phase 02]: The Codemasters $4000 bank comes from bits 6-0; bit 7 is only the RAM enable
- [Phase 02]: Writes while $3E disables the cartridge are ignored and recorded as unmapped_write (hardware docs silent; stated in src/bus.c)
- [Phase 02]: The Z80 is split into z80.c, z80_cb.c, z80_ed.c and z80_x3.c, each under the 800-line cap
- [Phase 02]: A save state refuses CPU combinations the core cannot reach and ties the beam to bus.at
- [Phase 03]: Save emitted pixels and the monotonic renderer cursor so partially rendered fields restore exactly.
- [Phase 03]: Validate the canonical absolute input body and exact record count before running; seal eight fixture samples only after sampling.
- [Phase 03]: Repository checks count complete source-policy, core ABI, JUnit, inventory, assertion and hook helpers by responsibility; packaged-output integration and operational exclusions are individual.
- [Phase 03]: Classify packaged-output integration, bootstrap, administration, publication and CI routing individually by actual responsibility.
- [Phase 03]: Share synthetic budget boundary cases between CI and hygiene while retaining their exact inventories.
- [Phase 03]: Keep GAME-01 and GAME-07 pending until complete phase-wide behavior is demonstrated.
- [Phase 03]: Use released frontend replay and an uninstalled initial-checkpoint module with unchanged production input/video source.
- [Phase 03]: Count the shared pixel oracle within the 300-line hosted budget and register its gate in CI and hygiene.
- [Phase 03]: GAME-07 has exact-SHA hosted input/scroll screenshot proof; GAME-04 remains pending for its full pad/profile/lag criteria.
- [Phase 03]: Use a saved monotonic dot cursor, tick-600 line hscroll/display latches and final-physical-line per-frame vscroll latch.
- [Phase 03]: Append SMS1=3; share VDP masks and retained-bus facts; R6 gates address A12/A11 (tile bits 7/6).
- [Phase 03]: Prepare bounded next-line SAT slots at tick 600; SMS1 width requires slot i+4; observe active-dot flags before clearing status.
- [Phase 03]: Retain 607/608 IRQ edges and independent pending sources; keep rolling NTSC-240 counter phase separate from fixed host deadlines.
- [Phase 03]: H origin uses qualified 32-tick inference; share effective TH mixing for rising capture and stable held reads.
- [Phase 03]: Latch height at final-line tick 600; publish separate bounded completed pixels and metadata without moving fixed host deadlines.
- [Phase 03]: GAME-02 mailbox criterion proven; GAME-01 hosted portable playing hashes passed in run 39494627790 (plan 03-09 summary); requirement disposition remains with phase verification.
- [Phase 03]: Append explicit video mode beyond previous media tail padding; save the held native LCD independently of full-field rendering.
- [Phase 03]: Preserve GG 6:5 native crop; resolve automatic host machine/region through the public cartridge probe and keep presentation independent.
- [Phase 03]: Reuse serialized pending/current absolute input for Pause reconnect arming; missing frontend callbacks are disconnected sources.
- [Phase 03]: Moldorian retains known battery identity and 24576-byte declaration with unsupported persistence geometry; no guessed battery span.
- [Phase 03]: Owned battery fixture is a fixed MIT hash fact consumed by production lookup and checked after linking independently of core targets.
- [Phase 03]: Card/cartridge aliases coalesce only when hardware facts agree; the shared SMS and GG SMS-mode image defaults to SMS.
- [Phase 03]: Battery spans use API 6 and transactional raw import API 7; existing ABI layouts and numeric values remain unchanged.
- [Phase 03]: Persistent extent accounting follows physical mapper translation; same-value mapped persistent writes increment generation, while import and resets preserve it.
- [Phase 03]: Raw import refuses known-none as no-content, unknown or unsupported persistence as unsupported-board, and supported oversize as save-size before reading bytes.
- [Phase 03]: Runner saves use same-directory POSIX rename or Windows MoveFileExW, acknowledging dirty generations only after checked write, flush, synchronization and close; Windows uses wide paths.
- [Phase 03]: Libretro SAVE_RAM delegates to the classified battery query only during the loaded lifetime; frontend restoration owns raw bytes and all filesystem I/O.
- [Phase 03]: Keep libretro_saves false and experimental status until separate hosted save-file and compatibility proof contracts are met.
- [Phase 03]: Reuse the existing classified battery fixture identity; its CRC-checked mailbox already reports the previous and incremented counter.
- [Phase 03]: Use bounded accepted predicates and a shared quoted CSV normalizer; keep expected values outside identity and healthy HALT outside boot-smoke rejection.
- [Phase 03]: Copy the base compatibility judge and adjacent shared parser before candidate checkout; compare eligible fresh fixture results in both directions.
- [Phase 03]: Compatibility exceptions name one unique old pass, require the actual new behaviour revision, and expire inclusively within 60 supplied-date days; GAME-06 remains pending through plan 03-17.
- [Phase 03]: Match fixtures by admitted SHA-256; run metadata misses as bounded unknown observations and restrict public updates to seeded CC0 cases with unchanged expectations.
- [Phase 03]: Keep playing performance report-only; reuse identical owned fixture, script and profile on comparable bases and report NO_BASE for older contracts.
- [Phase 03]: Use exclusive same-directory open for portable save temporaries, retaining private native permissions and untouched collision files.
- [Phase 03]: Stop execution with all eighteen plans complete, ready for code review before formal verification; phase 04 remains pending.
- [Phase 03]: Preserve every native and portable assertion while correcting actual inventories and measured host timeout bounds.

### Pending Todos

- [ci] Post-merge ci follow-ups (perf gate arming, nightly forced failure, Dependabot, wasm32 promotion): `.planning/todos/pending/post-merge-ci-follow-ups.md`

### Blockers/Concerns

- [Phase 3]: Research retains sourced 607/608 interrupt edges; H-counter coordinate is an explicit inference. Fixtures test the selected model; binary oracles only locate differences, hardware measurements remain v4.0.
- [Phase 3]: devkitSMS crt0 linking exception to be quoted and judged at the first third-party fixture admission.
- [Phase 3]: SMS Power! timing and GG port references were accessible during planning research; Moldorian's 24 KiB metadata is confirmed, physical bank decoding remains unresolved and must not be guessed.
- [Phase 4]: OPLL seed choice and FM:PSG mix need research during planning.
- [Phase 2]: Full sanitizer lanes are evidenced in CI; local Homebrew clang-21 ASan hangs at startup on macOS. Plan 03-12's focused battery replay and timed save fuzzing passed locally with the installed Apple sanitizer runtime.
- [Phase 3]: Deferred review info items IN-01..IN-06 (fault-record precision, unread bus.vdp_at, odd-size padding, probe UX, saved fault validation); see 02-REVIEW-DISPOSITION.md.

## Runtime Log

| Date | Step | Runtime | Model |
|------|------|---------|-------|
| 2026-10-18 | Roadmap (gsd-new-project) | Claude Code | claude-fable-5-1 |
| 2026-10-19 | Discuss phase 1 (gsd-discuss-phase) | Claude Code | claude-opus-5-5 |
| 2026-10-19 | Plan phase 1 (gsd-plan-phase) | Claude Code | claude-opus-5-5 |
| 2026-10-19 | Execute phase 1 (gsd-execute-phase) | Claude Code | claude-opus-5-5 |
| 2026-10-19 | Ship phase 1 (gsd-ship) | Claude Code | claude-opus-5-5 |
| 2026-10-19 | Discuss phase 2 (gsd-discuss-phase) | Claude Code | claude-opus-5-5 |
| 2026-10-19 | Plan phase 2 (gsd-plan-phase) | Claude Code | claude-opus-5-5 |
| 2026-10-19 | Execute phase 2 (gsd-execute-phase) | Claude Code | claude-opus-5-5 |
| 2026-10-20 | Ship phase 2 (gsd-ship) | Codex | GPT-6 |
| 2026-10-20 | Plan phase 3 (gsd-plan-phase) | Codex | GPT-6 |
| 2026-10-20 | Execute phase 3 through plan 03-18; ready for code review (gsd-execute-phase) | Codex | GPT-6 |

## Deferred Items

Items acknowledged and deferred at milestone close, most recent first:

| Category | Item | Status | Deferred At | Milestone |
|----------|------|--------|-------------|-----------|
| *(none)* | | | | |

## Session Continuity

Last session: 2026-10-20T23:04:04.634Z
Stopped at: Completed 03-18-PLAN.md; ready for code review; runtime Codex; model GPT-6
Resume file: None
