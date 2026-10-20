---
gsd_state_version: "1.0"
milestone: v1.0
milestone_name: SMS and Game Gear games play with sound in RetroArch
current_phase: 2
current_phase_name: The CPU matches the public vectors
status: "Phase 01 shipped — PR #4"
stopped_at: Phase 2 context gathered
last_updated: "2026-10-20T00:59:13.782Z"
last_activity: 2026-10-19
state_head: 6aea1b2951ccbdd35fd5ff43f71f778438c83c7f
progress:
  total_phases: 4
  completed_phases: 1
  total_plans: 12
  completed_plans: 12
  percent: 25
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-10-18)

**Core value:** Master System and Game Gear games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.
**Current focus:** Phase 01 — A test frame in RetroArch

## Current Position

Phase: 2 — The CPU matches the public vectors
Plan: Not started
Status: Phase 01 shipped — PR #4
Last activity: 2026-10-19

Progress: [███░░░░░░░] 25%

## Performance Metrics

**Velocity:**
- Total plans completed: 12
- Average duration: — min
- Total execution time: 0.0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01 | 12 | - | - |

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

### Pending Todos

- [ci] Post-merge ci follow-ups (perf gate arming, nightly forced failure, Dependabot, wasm32 promotion): `.planning/todos/pending/post-merge-ci-follow-ups.md`

### Blockers/Concerns

- [Phase 3]: VDP timing constants (frame/line interrupt tick, vertical scroll latch, SMS1 zoom slot rule, open-bus byte) conflict between sources; settled by fixtures against released binaries, hardware in v4.0.
- [Phase 3]: devkitSMS crt0 linking exception to be quoted and judged at the first third-party fixture admission.
- [Phase 2, 3]: SMS Power! returned 503 during preparation; facts taken from archived captures are flagged in SMSPOWER-EXTRACTS and re-read at the research steps.
- [Phase 4]: OPLL seed choice and FM:PSG mix need research during planning.

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

## Deferred Items

Items acknowledged and deferred at milestone close, most recent first:

| Category | Item | Status | Deferred At | Milestone |
|----------|------|--------|-------------|-----------|
| *(none)* | | | | |

## Session Continuity

Last session: 2026-10-20T00:59:13.710Z
Stopped at: Phase 2 context gathered
Resume file: .planning/phases/02-the-cpu-matches-the-public-vectors/02-CONTEXT.md
