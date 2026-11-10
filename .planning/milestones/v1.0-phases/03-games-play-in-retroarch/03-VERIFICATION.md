---
phase: 03-games-play-in-retroarch
verified: 2026-10-20T23:26:39Z
status: passed
score: 64/64 plan truths and 7/7 requirements verified
runtime: Codex
model: GPT-6
behavior_unverified: 0
overrides_applied: 0
human_verification: []
decision_coverage:
  honored: 28
  total: 28
  not_honored: []
covered_files:
  - ".env.example"
  - ".github/workflows/ci.yml"
  - ".github/workflows/nightly.yml"
  - ".planning/phases/03-games-play-in-retroarch/03-01-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-01-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-02-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-02-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-03-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-03-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-04-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-04-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-05-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-05-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-06-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-06-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-07-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-07-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-08-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-08-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-09-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-09-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-10-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-10-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-11-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-11-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-12-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-12-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-13-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-13-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-14-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-14-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-15-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-15-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-16-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-16-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-17-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-17-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-18-PLAN.md"
  - ".planning/phases/03-games-play-in-retroarch/03-18-SUMMARY.md"
  - ".planning/phases/03-games-play-in-retroarch/03-REVIEW-DISPOSITION.md"
  - ".planning/phases/03-games-play-in-retroarch/03-REVIEW-FIX.md"
  - ".planning/phases/03-games-play-in-retroarch/03-REVIEW.md"
  - ".planning/phases/03-games-play-in-retroarch/03-UAT.md"
  - "ASSET_POLICY.md"
  - "CMakeLists.txt"
  - "CMakePresets.json"
  - "PROVENANCE.md"
  - "README.md"
  - "THIRD-PARTY-NOTICES.md"
  - "cmake/CogmasterTests.cmake"
  - "docs/compatibility.md"
  - "docs/conformance.md"
  - "docs/embedding.md"
  - "docs/hardware/cartridges.md"
  - "docs/hardware/video.md"
  - "docs/maintaining.md"
  - "docs/runner.md"
  - "docs/testing.md"
  - "include/cogmaster.h"
  - "libretro/CMakeLists.txt"
  - "libretro/cogmaster_libretro.info.in"
  - "libretro/libretro.c"
  - "libretro/options.h"
  - "pins.txt"
  - "runner/args.c"
  - "runner/args.h"
  - "runner/exit_status.h"
  - "runner/expect.c"
  - "runner/expect.h"
  - "runner/input.c"
  - "runner/input.h"
  - "runner/main.c"
  - "runner/path.h"
  - "runner/probe.c"
  - "runner/save.c"
  - "runner/save.h"
  - "runner/sha1.c"
  - "runner/sha1.h"
  - "runner/stop.c"
  - "scripts/check-title.sh"
  - "scripts/compat-common.sh"
  - "scripts/compat-ratchet.sh"
  - "scripts/compat-update.sh"
  - "scripts/hygiene.sh"
  - "scripts/library.cmake"
  - "scripts/library.sh"
  - "src/battery.c"
  - "src/bus.c"
  - "src/cart.c"
  - "src/cartdb.c"
  - "src/cartdb.h"
  - "src/cartdb.inc"
  - "src/frame.c"
  - "src/instance.c"
  - "src/internal.h"
  - "src/io.c"
  - "src/mapper.c"
  - "src/sha1.c"
  - "src/sha1.h"
  - "src/state.c"
  - "src/vdp.c"
  - "src/vdp_render.c"
  - "tests/abi/tests.cmake"
  - "tests/api/v3.c"
  - "tests/api/v4.c"
  - "tests/api/v5.c"
  - "tests/api/v6.c"
  - "tests/api/v7.c"
  - "tests/api/v8.c"
  - "tests/built/expected.txt"
  - "tests/bus/test_vdp.c"
  - "tests/bus/tests.cmake"
  - "tests/cart/fixture.cmake"
  - "tests/cart/fixture_facts.txt"
  - "tests/cart/generator.cmake"
  - "tests/cart/test_cartdb.c"
  - "tests/cart/tests.cmake"
  - "tests/cmake/budgets.cmake"
  - "tests/cmake/ci_required.cmake"
  - "tests/cmake/expect_fail.cmake"
  - "tests/cmake/hashes.cmake"
  - "tests/cmake/repository.cmake"
  - "tests/cmake/repository.sh"
  - "tests/compat/allow.txt"
  - "tests/compat/ledger.csv"
  - "tests/compat/ratchet.cmake"
  - "tests/compat/tests.cmake"
  - "tests/compat/update.cmake"
  - "tests/core/test_core.c"
  - "tests/core/test_state.c"
  - "tests/diag/expected.csv"
  - "tests/fixtures/lines.c"
  - "tests/fixtures/play.c"
  - "tests/fixtures/play.h"
  - "tests/fixtures/tests.cmake"
  - "tests/fuzz/fuzz_run.c"
  - "tests/fuzz/fuzz_save.c"
  - "tests/fuzz/seeds.c"
  - "tests/fuzz/tests.cmake"
  - "tests/hygiene/fixtures/needs.txt"
  - "tests/hygiene/tests.cmake"
  - "tests/inventory/ci.txt"
  - "tests/inventory/fuzz.txt"
  - "tests/inventory/hygiene.txt"
  - "tests/inventory/library.txt"
  - "tests/inventory/nofp.txt"
  - "tests/inventory/wasm32.txt"
  - "tests/io/input_cases.h"
  - "tests/io/test_io.c"
  - "tests/io/tests.cmake"
  - "tests/library/tests.cmake"
  - "tests/libretro/battery.h"
  - "tests/libretro/controls.h"
  - "tests/libretro/host.c"
  - "tests/libretro/presentation.h"
  - "tests/libretro/tests.cmake"
  - "tests/mapper/test_battery.c"
  - "tests/mapper/tests.cmake"
  - "tests/perf/tests.cmake"
  - "tests/policy/budget_cases.cmake"
  - "tests/policy/e2e_cases.cmake"
  - "tests/policy/repository-checks.txt"
  - "tests/policy/tests.cmake"
  - "tests/retroarch/bmp.c"
  - "tests/retroarch/checkpoint.c"
  - "tests/retroarch/compare.c"
  - "tests/retroarch/flip.cmake"
  - "tests/retroarch/oracle.c"
  - "tests/retroarch/replay.c"
  - "tests/retroarch/run_retroarch.cmake"
  - "tests/retroarch/test.cfg.in"
  - "tests/roms/manifest.txt"
  - "tests/roms/src/battery.asm"
  - "tests/roms/src/battery.link"
  - "tests/roms/src/boot_to_play.asm"
  - "tests/roms/src/boot_to_play.link"
  - "tests/roms/src/boot_to_play_gg.asm"
  - "tests/roms/src/boot_to_play_gg.link"
  - "tests/roms/src/ports.asm"
  - "tests/roms/src/vdp_lines.asm"
  - "tests/roms/src/vdp_lines.link"
  - "tests/runner/expect.cmake"
  - "tests/runner/png_close_failure.c"
  - "tests/runner/test_save.c"
  - "tests/runner/tests.cmake"
  - "tests/tests.cmake"
  - "tests/vdp/test_background.c"
  - "tests/vdp/test_geometry.c"
  - "tests/vdp/test_gg.c"
  - "tests/vdp/test_sprites.c"
  - "tests/vdp/test_timing.c"
  - "tests/vdp/tests.cmake"
  - "tools/cartdb.c"
covered_digest: "v3:sha256:64978ca19ed483a3bad162522a8d99288f78ea44ac56a53c9b5e81c95e88cf29"
---

# Phase 03: Games play in RetroArch — Verification

**Goal:** Master System and Game Gear fixtures boot, draw, scroll, respond to two pads and keep battery saves across processes, in the runner and in one integrated RetroArch session, with frame hashes identical on every platform and a compatibility ledger that only ratchets up.

Initial verification, performed inline by Codex (GPT-6). The existing standard review and its one fix round preceded verification; CR-01 and WR-01 have fixed dispositions. No second code review or subsequent GSD step was invoked. The missing canonical report was regenerated through verify-work's documented verify-phase-goal path. AGENTS rule 6 assigns all checks and visual judgment here to the agent.

## Goal Achievement

| Requirement / observable truth | Status | Implementation and exercised evidence |
|---|---|---|
| GAME-01: Mode 4 tiles, sprites, scroll, priority, masks, zoom and 192/224/240 fields render deterministically | VERIFIED | `src/vdp_render.c` decodes all planes and attributes, bounds SAT work to 64 entries/eight slots and publishes immutable completed pixels. `tests/vdp/test_background.c`, `test_sprites.c`, `test_geometry.c` and independent `tests/fixtures/lines.c` exercise these rules. Six native lanes and be32 compare hashes; wasm32 is checked separately. |
| GAME-02: IRQ, counters and status use the selected sourced dot model | VERIFIED | `src/vdp.c` schedules ticks 600/604/607/608, recomputes gated pending IRQs, captures TH and catches up before status clear. `tests/vdp/test_timing.c` uses literal counter intervals and before/at/after boundaries; `vdp_lines` mailbox rows are CRC/value judged against `tests/diag/expected.csv`. |
| GAME-03: Native GG CRAM, crop, ports, Start and SMS mode work | VERIFIED | Native even-byte latch/odd-byte commit and held LCD live in the common VDP; `src/io.c` routes Start and aliases by resolved mode. `vdp.gg.*`, native owned-fixture tests and hosted 160×144 screenshot/oracle equality cover the native and SMS-mode paths. |
| GAME-04: Two pads, Pause, region/TV profiles and lag work end to end | VERIFIED | `cogmaster_set_input` validates absolute levels; frame-boundary application and port reads own Pause/reconnect/lag history. `tests/io/input_cases.h`, `tests/libretro/controls.h` and `tests/fixtures/play.c` exercise independent pads, hold/release/repress, reset semantics, disconnected callbacks, SOCD and per-frame lag. |
| GAME-05: Exact raw battery spans and atomic saves continue across processes | VERIFIED | `src/cartdb.c` classifies by image hash; `src/battery.c` exposes only supported persistence and validates imports before mutation. Mapper writes increment generation after physical translation. `runner/save.c` exclusively creates same-directory temporaries, checks write/flush/sync/close and atomically replaces before acknowledgment. Import/refusal, failure, collision, clean-flush, normal exit and signal continuation tests pass, including both native Windows lanes. |
| GAME-06: Compatibility facts ratchet; private library stays local; metadata remains experimental | VERIFIED | `scripts/compat-common.sh` bounds and normalizes quoted CSV; `compat-ratchet.sh` uses the preserved base helper closure in CI. Fresh fixture results, tamper controls, dated exceptions and generated-doc checks pass. `scripts/library.sh` executes supported metadata misses; `library.synthetic` checks every candidate and explicit zero coverage. `compat.public_update` rejects private labels/unknown identities/new goldens. `libretro.info_claims` keeps experimental status and unproven save-file claims false. |
| GAME-07: Released RetroArch receives input and displays independently correct scrolling output | VERIFIED | The hosted job records/replays frontend input through the unchanged production callback path, comparing baseline/changed SMS and native GG screenshots to the runner and `play_pixel`. A changed pixel and wrong-scroll control reject. The replay-only state bridge is uninstalled; production state support remains false. Complete hosted helper closure is 300 lines. |

## Plan Must-Haves

All 64 explicit plan truths are VERIFIED in their selected-model scope. All 50 artifact references exist and are substantive, and all 49 links are wired. Repeated artifacts are counted as plan references. The following matrix records the complete plan scope rather than treating SUMMARY claims as implementation evidence.

| Plan | Truths | Artifacts / links | Result and evidence |
|---|---:|---:|---|
| 03-01 | 4 | 6 / 3 | VERIFIED — VDP writes catch up the cursor; public frame/hash and loaded libretro consume it. Independent diamond pixels, mailbox counts and absolute-input boundary tests pass. |
| 03-02 | 2 | 2 / 2 | VERIFIED — repository closure inventory feeds strict line counting; original gates and negative controls execute. 528 check lines / 5,298 core lines is strictly below one tenth. |
| 03-03 | 3 | 1 / 2 | VERIFIED — released frontend replay delivers real input; screenshot comparison checks runner equality and independent scrolling pixels within the complete helper budget. |
| 03-04 | 4 | 2 / 2 | VERIFIED — shared SMS1 masks feed tile fetches; scroll/priority/mask and empty/unsupported cases execute with preserved raster intervals and canonical little-endian hashes. |
| 03-05 | 3 | 1 / 2 | VERIFIED — bounded SAT slots feed opaque-dot output/status; adjacency, transparency, SAT order, overflow and count-dependent SMS1 zoom have value-level tests. |
| 03-06 | 3 | 1 / 3 | VERIFIED — stamped events precede access; all six literal V tables and H/TH capture, reload, IE pending and clear races execute. |
| 03-07 | 3 | 3 / 2 | VERIFIED — latched height drives completed descriptor and independent six-mode guest pixels/mailboxes. Transition, canary, state-refusal and hostile deadline tests pass. |
| 03-08 | 3 | 3 / 3 | VERIFIED — resolved GG mode drives CRAM/ports/Start and native held image; crop, freeze/reset, state and owned guest tests pass. |
| 03-09 | 4 | 1 / 3 | VERIFIED — completed metadata and options feed geometry/video; both pixel formats, all heights, PAR/full-field options, rejected geometry and PAL rational silence tests pass. |
| 03-10 | 5 | 2 / 3 | VERIFIED — absolute records feed pin mixing/Pause arming and actual decoded reads feed lag; both pads, repeated levels, interleaving, restart and signal-stop tests pass. |
| 03-11 | 4 | 5 / 3 | VERIFIED — public image hash lookup feeds loader/probe facts; generator reproducibility, renamed battery identity and metadata refusal cases pass without fixture/core cycles. Moldorian remains explicitly unsupported. |
| 03-12 | 4 | 2 / 3 | VERIFIED — mapper physical offsets feed exact persistent extent/generation; import sizes, overlaps, repeat import, same-value writes and transactional refusals execute. |
| 03-13 | 4 | 3 / 3 | VERIFIED — bounded read/import and dirty flush acknowledgment feed checked replacement. Wide Windows paths, exclusive creation, collisions and every injected I/O failure preserve the expected files. |
| 03-14 | 3 | 3 / 3 | VERIFIED — guest counter becomes raw save, then fresh child mailbox. Missing/wrong saves differ; both signal paths and stable frontend SAVE_RAM/reset generations execute. |
| 03-15 | 5 | 6 / 3 | VERIFIED — bounded runner predicates feed normalized cases and deterministic docs. Quoting, empty ledger, malformed rows, aliases/order and honest smoke controls execute. |
| 03-16 | 3 | 2 / 3 | VERIFIED — preserved base judge compares normalized cases/fresh results; deleted passes, changed expectations/profiles/scripts and narrowly dated exception controls execute. |
| 03-17 | 3 | 4 / 3 | VERIFIED — literal local directory candidates feed bounded subprocesses and ignored observations; public update filtering and synthetic all-candidate/empty/infrastructure tests execute. |
| 03-18 | 4 | 3 / 3 | VERIFIED — complete playing hash matrix joins existing native/be32/wasm32 lanes; bounded allocation/state/fuzz tests, representative report-only workload, inventories and prior review fixes pass. |

All eighteen SUMMARY commit windows reconcile against their own recorded before/after SHAs, with the claimed counts and after-commit ancestry intact. Coverage classification yields 87 automated entries, zero present-to-owner entries and zero parse errors. UAT includes those 87 entries and one agent confirmation of the combined outcome.

## Required Artifacts and Data Flow

| Producer | Transform / consumer | Exercised result |
|---|---|---|
| Stamped VRAM/CRAM/register stores | monotonic renderer → completed pixels/descriptor → public frame → runner hash and libretro conversion | Independent per-coordinate SMS/GG and six-height oracle equality; immutable interval and transition tests |
| Absolute runner/frontend input | frame-boundary input → decoded pin reads → owned guest counter/scroll | Separate pad schedules, Start, Pause/reconnect, visible pixel changes and exact lag flags |
| Guest persistent write | physical mapper offset → classified span/generation → raw .srm → fresh process import | Predicted previous/incremented mailbox values and every raw save byte |
| Public hash metadata | sorted immutable lookup → resolved machine/mode/persistence | Renamed fixture identity, unsupported physical geometry and unknown/none controls |
| Ledger and owned fixture expectations | bounded quoted parser → fresh predicate execution → trusted base ratchet → generated docs | Four admitted fixture evaluations; tampered helper, expectation and identity controls reject |
| Local candidate directory | flat regular-image enumeration → bounded probe/run → ignored numbered sidecars | Known, unknown, unsupported, malformed and infrastructure classes; private-field filtering |
| Released RetroArch input replay | ordinary input callback → production guest run → screenshot | Baseline/changed SMS and GG runner equality plus independent oracle and pixel mutation rejection |

No requirement depends solely on agreement between two copies of production behavior. Frame agreement is supplemented by coordinate oracles; mailbox comparisons use independent literal values and CRC; save continuation uses separately predicted counters and raw bytes.

## Test Quality Audit

| Test family | Linked requirements | Active / skipped | Circular | Strongest assertion |
|---|---|---|---|---|
| VDP background/sprites/timing/geometry/GG and owned lines | GAME-01/02/03 | Active; 0 skipped | No | Literal pixels/counters and before/at/after transitions |
| I/O, frontend controls and owned play | GAME-03/04 | Active; 0 skipped | No | Independent pin/counter/lag values across press/hold/release and reset |
| Battery and runner save process cases | GAME-05 | Active; 0 skipped | No | Full bytes, generations, transactional refusal, fresh-process continuation and fault injection |
| Compatibility and synthetic local library | GAME-06 | Active; 0 skipped | No | Parsed values, distinct statuses and malicious-edit/privacy controls |
| Released frontend screenshot and comparator controls | GAME-03/07 | Active; 0 skipped | No | Every coordinate equals runner and independent oracle; mutations fail |

Disabled-test/debt-marker scan found no match in phase runtime and requirement-linked test modules. Expected fixture hashes are checked after assembly; expected pixel generation contains no production renderer calls. Hash equality proves portability in addition to, rather than replacing, independent pixel assertions. Production no-save-state and silent-audio paths are intentional milestone boundaries, tested as such.

## Behavioral Spot-Checks and Probe

The fresh full CI workflow is also the regression gate for phases 01/02: their vectors, bus, mapper, no-content card, ABI, packaging and policy cases remain registered and passing. Focused IRQ pending, height transitions, signal continuation, protected judge, failed PNG close and VDP fault-name checks additionally exercise state transitions and cleanup. The first attempted signal-test selector used `continue` instead of `continuation` and matched no tests; it was discarded as evidence, then the actual named test executed successfully.

The production GG fixture probe returned the correct 32 KiB image, valid region-6 header, GG/NTSC-export profile and admitted hashes. No separate declared probe script is missing. Exact native and hygiene JUnit inventories have no missing, duplicate, failed or skipped case.

## Decision Coverage

All trackable CONTEXT.md decisions are honored by shipped artifacts: 28 of 28, no not-honored entries. The two plan-03-17 prohibitions are VERIFIED by local-only sidecar handling, explicit public allowlisting and privacy controls, and by documentation that labels capped boots as smoke rather than gameplay/completion/sound support.

## Limits and Retained Assumptions

- This verifies the selected sourced timing model, not new hardware measurements. H-counter's 32-tick origin, tick-600 latches, NTSC-240 entry phase, SMS1 zoom slots and GG LCD freeze retain their recorded confidence limits.
- The generic `GAME-02/unclassified`, `GAME-03/unclassified` and `GAME-07/unclassified` probe assumptions, and plan 03-17's two descriptor-less privacy/transparency flags, remain visible. Concrete tests verify the specified behaviors; no claim is made that an unspecified generic edge was measured.
- Moldorian's 24,576-byte metadata remains identifiable with unsupported persistence geometry; no bank wiring is guessed.
- The dispatch's compatibility baseline is its selected SHA, so it verifies the protected path/control cases. PR base selection and prerelease rehearsal run at ship; merged release publication is confirmed there under AGENTS rules 6/10.
- Performance remains report-only; wasm32 is still report-only for the required roll-up. Their existing post-merge follow-up is unchanged.
- Only owned fixture screenshots are inspected. No private cartridge directory, ROM, BIOS or save is needed. The optional personal-hardware trial is outside UAT.

## Human Verification Required

None. Automated checks and agent inspection cover every acceptance criterion; no identity-bound consent or owner-hardware-only requirement applies. There are no unresolved behavioral gaps, overrides or skipped acceptance items.

## Documentation Reconciliation

Corrected two stale phrases in `docs/hardware/video.md`: collision bounds use the selected active height, and native GG LCD behavior points to the implemented crop/freeze model. These are prose-only corrections; runtime/build/test/workflow files remain byte-identical to the hosted revision. Local hygiene/doc checks ran after the correction.

## Verification Evidence

Commands ran from the repository root. Local rows have no hosted URL. Runtime logs, JUnit reports, hashes, images and screenshots stay under ignored build output; only commands, actual exit statuses and the run URL are recorded here.

| Command | Exit status | Run URL |
|---|---:|---|
| `cmake --workflow --preset ci` | 0 (342 tests + 4 package checks) | N/A (local) |
| `ctest --test-dir build/ci -R '^vdp.timing.irq_pending$' --output-on-failure` | 0 | N/A (local) |
| `ctest --test-dir build/ci -R '^vdp.geometry.transitions$' --output-on-failure` | 0 | N/A (local) |
| `ctest --test-dir build/ci -R '^compat.trusted_judge$' --output-on-failure` | 0 | N/A (local) |
| `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -DCHECK=check_junit -P tests/cmake/repository.cmake` | 0 | N/A (local) |
| `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -DCHECK=check_junit -P tests/cmake/repository.cmake` | 0 | N/A (local) |
| `build/ci/cogmaster-run --probe build/ci/fixtures/boot_to_play.gg` | 0 | N/A (local) |
| `git diff --check` | 0 | N/A (local) |
| `ctest --test-dir build/ci -R '^runner.save.continuation.term$' --output-on-failure` | 0 | N/A (local) |
| `cmake --workflow --preset hygiene` | 0 | N/A (local) |
| `ctest --test-dir build/ci -R '^runner.png_file$' --output-on-failure` | 0 | N/A (local) |
| `ctest --test-dir build/ci -R '^runner.trace.fault$' --output-on-failure` | 0 | N/A (local) |
| `cmake -DROOT=. -P tests/cmake/budgets.cmake` | 0 | N/A (local) |
| `cmake -DDIR=tests/retroarch -DCHECK=e2e_budget -P tests/cmake/repository.cmake` | 0 | N/A (local) |
| `git diff 8d9298db1a7c26b254b7fa076780e646eff0f631 --exit-code -- src runner libretro include tests .github CMakeLists.txt CMakePresets.json` | 0 | N/A (local) |
| `gh run view 39514577638 --exit-status` | 0 | https://github.com/szTheory/cogmaster/actions/runs/39514577638 |
| `gh run download 39514577638 --pattern 'hashes-*' --dir build/verify-03/hashes` | 0 | https://github.com/szTheory/cogmaster/actions/runs/39514577638 |
| `gh run download 39514577638 --name retroarch-owned-screenshots --dir build/verify-03/screenshots` | 0 | https://github.com/szTheory/cogmaster/actions/runs/39514577638 |
| `cmake '-DFILES=build/verify-03/hashes/hashes-be32/hashes.txt;build/verify-03/hashes/hashes-linux-arm64/hashes.txt;build/verify-03/hashes/hashes-linux-x64/hashes.txt;build/verify-03/hashes/hashes-macos-arm64/hashes.txt;build/verify-03/hashes/hashes-macos-x64/hashes.txt;build/verify-03/hashes/hashes-wasm32/hashes.txt;build/verify-03/hashes/hashes-windows-arm64/hashes.txt;build/verify-03/hashes/hashes-windows-x64/hashes.txt' -DCOUNT=8 -DMODE=compare -P tests/cmake/hashes.cmake` | 0 | https://github.com/szTheory/cogmaster/actions/runs/39514577638 |

Hosted revision: `8d9298db1a7c26b254b7fa076780e646eff0f631`, [CI run 39514577638](https://github.com/szTheory/cogmaster/actions/runs/39514577638). The required roll-up and all heavy jobs passed. Title, rehearsal and cleanup were correctly skipped for this dispatch; the diagnostic-only RetroArch log step was unnecessary after success. Both Windows build inventories executed the PNG regression. All four frontend screenshot/control steps succeeded. The agent inspected the four owned baseline/input SMS/GG PNGs downloaded from this run: native dimensions and the changed scroll/colour-counter patterns agree with the independently checked output.

## Gaps Summary

None. Phase 03 is verified and ready for ship. Phase 04 work has not started. Release rehearsal, merge and release confirmation belong to the separate ship step.

