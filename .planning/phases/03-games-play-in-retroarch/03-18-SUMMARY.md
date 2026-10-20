---
phase: 03-games-play-in-retroarch
plan: "18"
plan_head_before: ab2cba4975913f2acf039faccebb8d21c8a8e416
plan_head_after: dd40d2f54987a149917a4f281f9d6edab9280fe5
status: complete
subsystem: playing-determinism-and-phase-integration
tags:
  - portable-hashes
  - bounded-execution
  - performance
  - wasi
requires:
  - phase: 03-17
    provides: Complete playing, persistence, compatibility and local-library paths
provides:
  - Sixty-six playing-output rows identical across six native platforms, be32 and wasm32
  - Loaded-content allocation, state-refusal, instance-isolation and hostile-run regressions
  - Scripted playing performance workload with unchanged report-only policy
  - Fresh hosted platform, native-save, trusted-judge and RetroArch integration evidence
  - Documentation and execution handoff ready for code review before phase verification
affects:
  - phase-03-code-review
  - phase-03-verification
tech-stack:
  added: []
  patterns:
    - Canonical crop-word hashes and independent fixture pixel oracles
    - Shared explicit playing profile, image and script for performance comparisons
    - Exclusive same-directory temporary creation without unavailable WASI mkstemp
key-files:
  created: []
  modified:
    - tests/cmake/hashes.cmake
    - tests/runner/tests.cmake
    - tests/core/test_core.c
    - tests/core/test_state.c
    - tests/fuzz/fuzz_run.c
    - tests/fuzz/seeds.c
    - tests/perf/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/nofp.txt
    - runner/save.c
    - src/cartdb.h
    - .github/workflows/ci.yml
    - .github/workflows/nightly.yml
    - docs/testing.md
    - docs/maintaining.md
key-decisions:
  - Keep performance report-only and wasm32 promotion in the existing post-merge follow-up.
  - Compare head and base using the same owned image, input script and explicit SMS2/SMS-video/NTSC-export profile; report NO_BASE for older renderer/input contracts.
  - Preserve all native and portable assertions while correcting actual inventories and measured host timeout bounds.
  - Stop at phase 03 ready for code review; formal verification and phase 04 remain separate steps.
requirements-completed:
  - GAME-01
  - GAME-02
  - GAME-03
  - GAME-04
  - GAME-05
  - GAME-06
  - GAME-07
coverage:
  - id: playing-portability
    description: Complete playing crops and fixture images compare identically on all eight existing platforms
    requirement: GAME-01
    verification:
      - kind: integration
        ref: runner.hashes_file and CI hash-equality in run 39511836104
        status: pass
      - kind: integration
        ref: Eight downloaded hashes.txt files compared with tests/cmake/hashes.cmake
        status: pass
    human_judgment: false
  - id: selected-timing-model
    description: Existing IRQ, counter, TH capture, status-clear and mailbox checks remain green across portable lanes
    requirement: GAME-02
    verification:
      - kind: unit
        ref: tests/vdp/test_timing.c and tests/diag/expected.csv through ci and wasm32
        status: pass
    human_judgment: false
  - id: native-and-sms-mode-gg
    description: Native GG Start and crop plus extended-height crop/freeze and GG SMS-mode output remain deterministic
    requirement: GAME-03
    verification:
      - kind: unit
        ref: vdp.gg.freeze and vdp.gg.state
        status: pass
      - kind: e2e
        ref: CI RetroArch Game Gear native crop screenshot equality in run 39511836104
        status: pass
    human_judgment: false
  - id: independent-input
    description: Both SMS pads, GG Start, region profiles, reset semantics and guest-read lag accounting retain their owned-fixture proofs
    requirement: GAME-04
    verification:
      - kind: integration
        ref: io.absolute_input, io.lag_decode, libretro.controls.rgb565 and libretro.controls.xrgb8888
        status: pass
      - kind: integration
        ref: Scripted playing hash rows and independent fixture oracles
        status: pass
    human_judgment: false
  - id: native-and-wasi-persistence
    description: Atomic save replacement, collision preservation, failure cleanup and fresh-process continuation run on native Windows and portable WASI
    requirement: GAME-05
    verification:
      - kind: integration
        ref: runner.save replacement, failure and continuation cases in run 39511836104
        status: pass
    human_judgment: false
  - id: compatibility-and-library
    description: Quoted predicates, fresh fixture results, protected comparison and private library classification retain exact executed inventories
    requirement: GAME-06
    verification:
      - kind: integration
        ref: compat.fixtures, compat.ratchet, compat.trusted_judge and library.synthetic
        status: pass
      - kind: integration
        ref: CI protected compatibility comparison on dd40d2f in run 39511836104
        status: pass
    human_judgment: false
  - id: released-frontend-play
    description: The released frontend delivers input and scrolling output matching runner and independent SMS/GG pixel expectations
    requirement: GAME-07
    verification:
      - kind: e2e
        ref: CI RetroArch SMS input and scrolling screenshot equality in run 39511836104
        status: pass
      - kind: other
        ref: Agent inspection of final owned input.png and gg-input.png artifacts
        status: pass
    human_judgment: false
  - id: bounded-loaded-execution
    description: Loaded content allocates nothing after load, preserves an interleaved neighbour, refuses invalid video state and stays within fixed deadlines and sprite bounds
    requirement: GAME-01
    verification:
      - kind: unit
        ref: core.no_alloc_after_load and core.state.record_sweep
        status: pass
      - kind: integration
        ref: fuzz.regress.run, both nofp lanes and ASan in run 39511836104
        status: pass
    human_judgment: false
  - id: representative-workload-and-budgets
    description: The playing benchmark reaches an independent expected frame while repository, hosted-helper and documentation budgets remain strict
    requirement: GAME-01
    verification:
      - kind: integration
        ref: perf.playing and CI perf control in run 39511836104
        status: pass
      - kind: other
        ref: docs.budgets, policy.e2e_budget and hygiene
        status: pass
    human_judgment: false
actuals:
  tokens: 13566
  tasks: 2
  commits: 9
duration: 31min
completed: 2026-10-20
---

# Phase 3 Plan 18: Playing Portability and Review Handoff Summary

Sixty-six playing-output rows now match on all eight platforms, with loaded-content boundedness, portable saves and a scripted representative benchmark.

## Accomplishments

Hash generation includes all twelve built SMS/GG images with unambiguous filenames. Playing output covers
SMS1/SMS2, native GG and GG SMS mode; supported export/Japan/PAL profiles; released input, each pad, both
pads and native Start; and native GG 224/240-line crop/freeze output. PAL GG remains correctly refused.
Canonical RGB444 crop words stay little-endian; host aspect/filter conversion stays outside hashes. Card
pins and independent glyph/line pixel oracles remain unchanged. A recurring empty-fixture control preserves
the optional card-only path.

The allocation test loads content, hammers VRAM/CRAM/registers and absolute input across supported profiles,
checks fixed deadlines with at most 69 ticks overshoot, bounded sprite scans and a monotonic renderer cursor,
and interleaves another instance without shared-state changes. Enabled/populated video joins transactional
state refusal sweeps. Fuzz replay checks allocation counts, deadline/raster/sprite bounds and a new generated
renderer-hammer seed. Exact nofp inventories include API consumers 6, 7 and 8.

The existing Cachegrind and nightly plumbing now uses boot_to_play with explicit SMS2, SMS video and NTSC
export plus button-1 presses in frames 5, 6 and 8. Head/base share the same head-built image and script;
bases without API 5, behaviour 8 or the fixture source report NO_BASE. The bench preset remains small; CI
builds the owned fixture target separately. The generator uses linear string repetition for long scripts.
perf.playing compares its bench frame against the independent pixel oracle. Report-only arming, wasm32
promotion and other post-merge follow-ups remain unchanged. Nightly timeouts are provisional for the more
representative workload; no hosted nightly run or gate-arming claim is made.

Runner, embedding, conformance, maintainer and preparation docs now describe implemented API 8/state 10,
metadata/header auto resolution, input, resets, PAR/GG options, raw saves and quoted compatibility behavior.
The agent inspected final owned SMS/GG screenshots against the code and expected counter/scroll output.
No owner UAT, dependency, new hosted helper or additional GSD step was introduced.

## Task Commits

1. Task 1 RED — `7364603`: require the missing complete playing matrix.
2. Task 1 GREEN — `5317ec2`: extend deterministic loaded-content and hash checks.
3. Task 2 — `728c24d`: wire the scripted playing workload and reconcile documentation.
4. Integration — `9b791f5`: fix GCC byte conversions, immutable ELF metadata storage and Windows LF generation.
5. Matrix completion — `d36c98a`: include native GG extended-height output and unique fixture image labels.
6. Host timing — `f46c237`: use the observed Windows ARM ratchet timeout bound.
7. Inventory — `82ffc03`: include both registered WASI compatibility checks.
8. Optional path — `a4c38c8`: retain card-only hashes with fixtures disabled and add its recurring control.
9. WASI save path — `dd40d2f`: replace unavailable mkstemp with exclusive open and collision preservation.

Started 2026-10-20T22:31:23Z; hosted evidence completed 2026-10-20T23:01:38Z. Twenty-seven implementation,
test and documentation files changed. Actual tokens are realized diff characters divided by four, rounded
up; commits are measured from the persisted ledger through the final production commit. No files deleted.
Runtime: Codex. Model: GPT-6.

## Evidence

All final hosted evidence targets `dd40d2f54987a149917a4f281f9d6edab9280fe5` in
[CI run 39511836104](https://github.com/szTheory/cogmaster/actions/runs/39511836104).

| Command | Exit | Result |
|---|---:|---|
| cmake --workflow --preset ci | 0 | 342 tests plus four package checks |
| cmake --workflow --preset hygiene | 0 | 38 tests |
| sh scripts/hygiene.sh --tree | 0 | Tree privacy, licence and identity checks |
| cmake -DROOT=. -P tests/cmake/budgets.cmake | 0 | 528 complete check lines / 5,298 src lines, strictly below one tenth |
| cmake -DDIR=tests/retroarch -DCHECK=e2e_budget -P tests/cmake/repository.cmake | 0 | 300 hosted helper lines, at most 300 |
| cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -DCHECK=check_junit -P tests/cmake/repository.cmake | 0 | Native executed inventory matched |
| cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -DCHECK=check_junit -P tests/cmake/repository.cmake | 0 | Hygiene executed inventory matched |
| gh workflow run ci.yml --ref phase/03-games-play-in-retroarch | 0 | Final exact-SHA CI required, six native builds, ASan, both nofp, be32, hash equality, perf and RetroArch passed |
| cmake --workflow --preset wasm32 | 0 | 272 portable tests; all 326 configured registrations match the actual inventory; hashes equal Linux x64 |
| gh run download 39511836104 --pattern hashes-* --dir build/hosted-03-18-final | 0 | Eight fresh lane hash files |
| cmake '-DFILES=build/hosted-03-18-final/hashes-linux-x64/hashes.txt;build/hosted-03-18-final/hashes-linux-arm64/hashes.txt;build/hosted-03-18-final/hashes-macos-x64/hashes.txt;build/hosted-03-18-final/hashes-macos-arm64/hashes.txt;build/hosted-03-18-final/hashes-windows-x64/hashes.txt;build/hosted-03-18-final/hashes-windows-arm64/hashes.txt;build/hosted-03-18-final/hashes-be32/hashes.txt;build/hosted-03-18-final/hashes-wasm32/hashes.txt' -DCOUNT=8 -DMODE=compare -P tests/cmake/hashes.cmake | 0 | Eight byte-identical files containing all 66 playing rows |
| gh run download 39511836104 --name retroarch-owned-screenshots --dir build/hosted-03-18-final-screenshots | 0 | Final SMS/GG input screenshots inspected by the agent |
| cmake -DCASE=workload -DDIR=build/ci/perf-final -DCOUNTS=300000 -P tests/perf/tests.cmake | 0 | Full deterministic input script generated |
| build/ci/cogmaster-run build/ci/fixtures/boot_to_play.sms --machine sms --region ntsc-u --video-mode sms --input build/ci/perf-final/300000.input --bench 300000 | 0 | Full playing workload completed with lag_frames=4 |
| git diff --check | 0 | No whitespace errors |

The final hosted Cachegrind control reports 11,879,630 Ir/frame three times with spread 0. Performance remains
report-only. Windows ARM64 ran all 342 tests and four package checks, including native replacement, signal
continuation and refusal controls; its ratchet took 28.25 seconds. The trusted baseline judge and complete
helper closure were copied before candidate checkout and the protected comparison passed on the final SHA.
This dispatch compares that SHA to itself; PR base protection becomes relevant when a PR supplies its base.

Raw logs, JUnit reports, ROM images and screenshots remain ignored runtime output, never committed logs.
The tracer gate reran the full CI workflow before Task 2. Optional local WASI configure and known-broken
Homebrew ASan startup were not repeated; their actual hosted lanes passed.

## TDD Gate Compliance

Task 1 first ran runner.hashes_file with the new GG input-row assertion; CTest exited 8 because that row was
absent. The fresh unchanged JUnit report passed check tdd-red-evidence with RED_EVIDENCE_OK. Semantic
inspection confirmed the named assertion ran after successful generation, with no setup/discovery failure.
The RED commit precedes the GREEN coverage commit. GREEN uses a test commit because the scoped behavior
already existed and this task adds integration proof. Task 2 is benchmark/configuration/documentation glue.
Global TDD mode remains false. No required gate or test was skipped.

## Deviations from Plan

- **[Rule 3 - Hosted integration]** GCC rejected five implicit byte conversions in the existing controls
  harness, and ELF classified pointer-relocated constant metadata as data. Explicit bounded byte casts and
  inline immutable SHA/title arrays matching the converter's existing 41/256 bounds restore all platform
  checks without checker exemptions or public ABI changes. Windows-generated expectation files now use
  canonical LF while malformed/unterminated controls retain exact bytes. Fixed in `9b791f5`.
- **[Rule 3 - Host harness runtime]** Windows ARM64 exceeded the ratchet's original 30-second timeout;
  the following passing run took 30.56 seconds. Raised that test alone to 60 seconds, preserving every
  assertion. The final run took 28.25 seconds. Fixed in `f46c237`.
- **[Rule 1 - Optional fixture path]** The new wrapper supplied empty FIXTURES as a defined variable,
  which could break a fixtures-disabled build. Test the nonempty value and exercise the card-only path
  during the recurring hash check. Fixed in `a4c38c8`.
- **[Rule 3 - WASI host API]** WASI libc omits mkstemp, as documented in its
  [official issue](https://github.com/WebAssembly/wasi-libc/issues/229). Use exclusive same-directory
  open with native mode 0600, skip existing names without truncation, and retain rename replacement.
  Existing collision, failure, cleanup and continuation cases pass on WASI and native hosts. Native POSIX
  sync and Windows durability behavior remain intact; WASI durability belongs to its host. Fixed in `dd40d2f`.

The broken-windows register records and resolves these integration deviations; zero open entries remain.
Actual missing WASI registrations were added rather than exempted. No package installation or auth gate occurred.

## Known Stubs

None. No skipped test or unrun required verification remains.

## Threat Flags

| Flag | File | Description |
|---|---|---|
| threat_flag: file-access | runner/save.c | Exclusive same-directory temporary creation replaces unavailable mkstemp; collisions remain untouched, native permissions stay private and all replacement/failure/cleanup controls pass |

## Confidence and Handoff

The H-counter's 32-tick origin translation remains an explicit coordinate inference. Tick-600 latches and
native GG 240-line LCD freeze are selected sourced models, with lower confidence than measured 607/608 IRQ
edges. Tests and binary agreement establish implementation behavior, not new hardware measurements.
Moldorian's pinned 24 KiB declaration remains identifiable with unsupported persistence geometry; no mask or
modulo decode is guessed. Retain the three generic probe provenance flags GAME-02/unclassified,
GAME-03/unclassified and GAME-07/unclassified, plus the two descriptor-less privacy/transparency flags from
plan 17. This plan does not silently resolve them or manufacture owner UAT.

Execution is complete for all eighteen plans. Phase 03 remains pending formal code review and verification;
phase 04 is not advanced. The exact next command is `$gsd-code-review 03`. Code review precedes verification,
with at most two fix rounds and one new-evidence gap round; tooling findings become seeds. No subsequent
GSD step, PR, ship operation or gate arming was invoked.

## Self-Check: PASSED

All twenty-seven changed implementation/test/documentation files and this summary exist. All nine task
commits are ancestors of HEAD. The exact final native and hygiene inventories pass, all eight downloaded
hash files compare equal, the complete hosted workflow succeeds and the broken-windows register has zero
open entries. No source file was deleted and no owner action remains.
