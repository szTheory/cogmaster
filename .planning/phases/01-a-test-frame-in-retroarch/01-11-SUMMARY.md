---
phase: 01-a-test-frame-in-retroarch
plan: 11
subsystem: ci-hosted-run
status: complete
tags:
  - ci
  - hosted-run
  - msvc
  - retroarch
  - rehearsal
requires:
  - "01-09: ci.yml with six build legs, the portability lanes, hash-equality, retroarch, perf and CI required"
  - "01-10: the rehearsal and cleanup jobs, publish.sh rehearsal-create and rehearsal-delete"
provides:
  - "Draft probe pull request #3 (ci: probe the phase branch before ship), left open for plan 12 to close"
  - "A green ci.yml run for bc256f2: https://github.com/szTheory/cogmaster/actions/runs/39335167905"
  - "tests/cmake/abi.cmake reads MSVC's own constant names (??_C@_, __xmm@) and the x64 vectorizer's __isa_available pair"
  - "Per-job durations of the green run, for plan 12's measured timeouts"
affects:
  - "plan 12: closes PR #3, replaces the provisional timeouts with the durations below, applies the ruleset handoff"
actuals:
  tokens: 1400
  tasks: 2
  commits: 1
plan_head_before: f81436f5629c5877b23dc53a4e3efad6e3dbcb11
plan_head_after: bc256f2c1bb73c46574aa4d247a04461a8b5495b
tech-stack:
  added: []
  patterns:
    - "A per-system table of compiler-generated name regexes sits beside the helper allowlist in abi.cmake; such names are dropped from the globals and symbols checks only on the system that emits them"
key-files:
  created: []
  modified:
    - tests/cmake/abi.cmake
decisions:
  - "MSVC string-literal (??_C@_) and vector-constant (__xmm@) COMDATs are exempt from the globals and symbols checks on Windows; __isa_available (undefined) and __isa_available_default (BSS) are allowed on Windows x64 only, matching BoringSSL's util/audit_symbols.go Windows list. __real@ (floating-point constants) stays forbidden"
  - "The merge ref was accepted as the rehearsal's target_commitish; no checkout switch to the head sha was needed"
  - "release.yml's concurrency.queue: max is untouched; release.yml runs only on push to main, so this run cannot show whether GitHub accepts it (confirmed at ship)"
requirements-completed:
  - FRAME-02
  - FRAME-04
  - FRAME-06
  - FRAME-07
  - FRAME-08
  - FRAME-09
coverage:
  - id: D1
    description: "On the draft pull request the ci.yml run for HEAD bc256f2 has CI required concluded success, with all six build legs (Linux x64/arm64, macOS arm64/x64, Windows x64/arm64) green"
    requirement: FRAME-01
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39335167905 (plan verify command 1, CI required conclusion grep, exit 0)"
        status: pass
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39335167905 (plan verify command 2, six build legs all success, exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "hash-equality compared seven hashes.txt files (six legs and be32) and printed 7 identical files; wasm32 printed that its hashes.txt equals Linux x64's"
    requirement: FRAME-02
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39335167905 jobs hash-equality and wasm32"
        status: pass
    human_judgment: false
  - id: D3
    description: "abi.undefined, abi.globals and abi.symbols pass on both MSVC legs once MSVC's own constant names are recognised, and still fail on a writable counter and on the x64-only names elsewhere"
    requirement: FRAME-04
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39335167905 jobs build (windows-x64) and build (windows-arm64)"
        status: pass
      - kind: other
        ref: "cmake -P tests/cmake/abi.cmake against a stub nm printing the MSVC names: Windows/AMD64 undefined and globals exit 0; Windows/ARM64 and Linux/x86_64 exit 1; with an extra counter B symbol, globals exit 1"
        status: pass
      - kind: integration
        ref: "tests/abi/tests.cmake#abi.globals.self_test"
        status: pass
    human_judgment: false
  - id: D4
    description: "The asan, both nofp, be32 and wasm32 lanes ran green on the hosted runners"
    requirement: FRAME-04
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39335167905 jobs asan, nofp (ubuntu-26.04), nofp (ubuntu-26.04-arm), be32, wasm32"
        status: pass
    human_judgment: false
  - id: D5
    description: "hygiene and title ran green inside CI required"
    requirement: FRAME-05
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39335167905 jobs hygiene and title"
        status: pass
    human_judgment: false
  - id: D6
    description: "The rehearsal published rehearsal-39335167905-1 with the App token (18 archives from the merge commit, SHA256SUMS, 19 assets, every download matching SHA256SUMS and verifying, each tampered copy failing), and cleanup deleted the release and its tag"
    requirement: FRAME-06
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39335167905 jobs rehearsal and cleanup (plan verify command 3, success,success, exit 0)"
        status: pass
      - kind: other
        ref: "gh release view rehearsal-39335167905-1 (non-zero) and gh api repos/szTheory/cogmaster/git/ref/tags/rehearsal-39335167905-1 (non-zero); plan verify command 4 exit 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "The retroarch job compared RetroArch 1.22.2's screenshot with cogmaster-run --png and printed 256x192, every pixel equal; flip.cmake saw compare exit 1 at pixel 7,3 on the flipped copy. The macOS arm64 leg installed the core and .info with the README line and the checksum refused the tampered zip"
    requirement: FRAME-07
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39335167905 jobs retroarch and build (macos-arm64)"
        status: pass
    human_judgment: false
  - id: D8
    description: "Every build leg built and tested the installed-package, C++ header and add_subdirectory consumers"
    requirement: FRAME-08
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39335167905 six build jobs"
        status: pass
    human_judgment: false
  - id: D9
    description: "perf measured the head at 2146896 Ir/frame in report mode and gave verdict NO_BASE, because main (merge base 75a36f4) has no bench preset yet; the merge-base measurement starts on the first pull request after this phase merges"
    requirement: FRAME-09
    verification:
      - kind: e2e
        ref: "https://github.com/szTheory/cogmaster/actions/runs/39335167905 job perf"
        status: pass
    human_judgment: false
duration: 13min
completed: 2026-10-19
---

# Phase 1 Plan 11: Hosted CI Green on the Probe Pull Request Summary

**All 20 jobs of `ci.yml` passed on the hosted runners for `bc256f2`, including `CI required`, after one fix:
`abi.cmake` now recognises MSVC's own constant names. Seven hash files are identical, wasm32 matches Linux x64,
RetroArch's screenshot equals the runner's frame at 256x192, and the App-token rehearsal published, verified and
deleted `rehearsal-39335167905-1`.**

## Run

- Draft PR: https://github.com/szTheory/cogmaster/pull/3 (`ci: probe the phase branch before ship`), left open
  for plan 12.
- Green run: https://github.com/szTheory/cogmaster/actions/runs/39335167905 (head `bc256f2`).
- First run, red on both Windows legs: https://github.com/szTheory/cogmaster/actions/runs/39334762778 (head
  `f81436f`).
- Nightly bench also ran on the PR because `nightly.yml` changed. Run 39335167849 succeeded.

## Verification (command, exit status)

- `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` before each push: exit 0.
- Plan verify 1 (`CI required` concluded success for HEAD's run): exit 0.
- Plan verify 2 (exactly six `build` jobs, all success): exit 0.
- Plan verify 3 (`rehearsal`,`cleanup` = `success,success`): exit 0.
- Plan verify 4 (no release and no tag `rehearsal-39335167905-1`): exit 0.
- Job-name greps for `asan`, `nofp` (two jobs), `be32`, `wasm32`, `hash-equality`, `retroarch`, `perf`, `hygiene`,
  `title`, `changes`, `rehearsal`, `cleanup` and `CI required` against `gh run view 39335167905 --json jobs`: each
  exit 0.
- `git log --format=%B origin/main..HEAD > build/commit-bodies.txt`: exit 0. The skip-token grep on that file:
  exit 1, so no commit body holds a skip token.

## Evidence from the green run

- **retroarch:** `256x192, every pixel equal`. `flip.cmake` saw `compare exited 1: first difference at 7,3`.
- **hash-equality:** `hashes: 7 identical files`, from six legs plus be32.
- **wasm32:** `wasm32 hashes.txt equals Linux x64's`. The step reports only and is not in `CI required`.
- **macOS arm64 install line:** the core and its `.info` were installed. With `FLIP=1`, the checksum refused the
  tampered zip (exit 1, as expected).
- **perf:** `perf testcard head=2146896 base=none delta_bp=none verdict=NO_BASE`, in report mode. The merge base
  `75a36f4` on main has no bench preset yet.
- **rehearsal:** tag `rehearsal-39335167905-1`.
  - `prepare` printed: 18 archives from `ffdb671` (the PR merge commit, accepted as `target_commitish`), and
    `SHA256SUMS` written.
  - The release was published with 19 assets. Every download matched `SHA256SUMS` and verified.
  - The attested step flipped one byte in a copy of each archive; every flipped copy failed, as expected.
- **cleanup:** `rehearsal-39335167905-1 and its release are gone`. After cleanup, `gh release view` and
  `gh api .../git/ref/tags/rehearsal-39335167905-1` both exit non-zero.

## Job durations (seconds, green run, for plan 12's timeouts)

title 5, changes 4, hygiene 16, be32 32, asan 22, build linux-arm64 24, build macos-arm64 23, build windows-x64
45, build macos-x64 34, nofp ubuntu-26.04-arm 6, build linux-x64 25, nofp ubuntu-26.04 6, build windows-arm64 74,
perf 19, wasm32 21, rehearsal 365, hash-equality 6, retroarch 18, cleanup 12, CI required 2.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The link-surface checks rejected MSVC's compiler-generated names**
- **Found during:** Task 1, first hosted run 39334762778.
- **Issue:** On windows-x64, `abi.undefined` failed on `__isa_available`, `abi.globals` on
  `B __isa_available_default`, and `abi.symbols` on 30 `??_C@_` string literals and two `__xmm@` constants. On
  windows-arm64, only `abi.symbols` failed, on the string literals. These are names MSVC gives its own read-only
  COMDAT constants and its x64 vectorizer's CPU check. They are not core state and not part of the public API.
- **Fix:** `abi.cmake` changed in two places:
  - A new `generated` table (system regex : name regex) drops `??_C@_` and `__xmm@` on Windows, and
    `__isa_available_default` on Windows x64, from the globals and symbols checks. The names are still counted as
    defined.
  - A new helper row allows `__isa_available` on Windows x64.

  The comment cites BoringSSL's `util/audit_symbols.go`, which lists the same names for MSVC. `__real@`
  (floating-point constants) stays forbidden.
- **Files modified:** tests/cmake/abi.cmake (the plan's file list did not name it)
- **Commit:** bc256f2

### Notes

- Task 2 needed no code change: the rehearsal and cleanup passed on the same run, so it only recorded evidence and
  made no commit.
- The plan commit ledger file was not written before the first commit. `plan_head_before` is `f81436f`, the
  branch head when the plan started, and `git rev-list --count f81436f..bc256f2` prints 1.
- `release.yml`'s `concurrency.queue: max` was not exercised: the workflow runs only on push to main.

## Known Stubs

None.

## Self-Check: PASSED

- FOUND: tests/cmake/abi.cmake
- FOUND: bc256f2 (ancestor of HEAD)
- FOUND: run 39335167905 concluded success for bc256f2
