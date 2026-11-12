---
phase: 10-bios-boot
plan: "03"
subsystem: public-header
tags:
  - bios
  - api
  - abi
  - media-struct
status: complete

requires:
  - 10-02
provides:
  - cogmaster_media carries reserved3, bios and bios_size after fm (72 bytes on 64-bit, 40 on 32-bit)
  - API 14, behaviour revision 16 and state format 16 with ABI 1
  - tests/api/v14.c and abi.api_v14 on the ci, wasm32 and nofp lanes
  - the real commit-order evidence that both firmware scans precede the first code commit
affects:
  - 10-04
  - 10-05

actuals:
  tokens: 9000
  tasks: 2
  commits: 2

plan_head_before: df5d0bf74e065cdb0c8b14b0581b26bf9d8432a0
plan_head_after: 4fa66aebf254d34120d809080071d8a4a5c9637e
commits: 2

tech-stack:
  added: []
  patterns:
    - "An appended struct field loosens only the size pin of the struct (>=) and leaves every offsetof pin exact, so a moved field still fails"
    - "The previous version's test keeps its offsets and sizes with == and its version numbers with >=, so each vK.c pins what its version added"

key-files:
  created:
    - tests/api/v14.c
  modified:
    - include/cogmaster.h
    - src/internal.h
    - tests/api/v9.c
    - tests/api/v13.c
    - tests/core/test_core.c
    - tests/abi/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/nofp.txt

key-decisions:
  - "The three new fields are read nowhere in src: the load path, the state and the frame are unchanged, so no hash, fault or state byte moved and the whole ci workflow is green at both commits"
  - "The layout asserts sit beside the existing media asserts and use the same UINTPTR_MAX condition as the header for the 64-bit-only pad, because the preprocessor cannot evaluate sizeof"

requirements-completed:
  - BIOS-01

coverage:
  - id: E1
    description: "cogmaster_media ends with reserved3 (64-bit only, offset 52), bios (56 on 64-bit, 32 on 32-bit) and bios_size (64 / 36), sizeof 72 / 40, with the three versions at 14, 16 and 16 and ABI 1, and every other struct size unchanged"
    requirement: BIOS-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^abi\\.api_v14$' exit 0; static asserts in src/internal.h compile on 64-bit; container gcc:13 compile of v14.c exit 0 (32-bit asserts are checked by the be32 lane)"
        status: pass
    human_judgment: false
  - id: E2
    description: "The shape-only pins moved and nothing else: sizeof(cogmaster_media) in v9.c and test_core.c and the three version macros plus v.api, v.behaviour and v.state in v13.c are compared with >=; every offsetof pin and every other size pin stays =="
    requirement: BIOS-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^(abi\\.|core\\.|docs\\.api_names)' exit 0 (53 tests); git diff --name-only origin/main -- src prints only src/internal.h; git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake exit 0"
        status: pass
    human_judgment: false
  - id: E3
    description: "abi.api_v14 is registered beside abi.api_v13 on ci, wasm32 and nofp only, and a version bump nobody pinned turns abi.api_version red"
    requirement: BIOS-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -N -R '^abi\\.api_v1[0-4]$' prints Total Tests: 5; python edit of the API version to 15 made ctest -R '^abi\\.api_version$' exit 8, restored exit 0; cmake --workflow --preset ci exit 0"
        status: pass
    human_judgment: false
  - id: E4
    description: "The commits that added the digest table, the release firmware scan and the window scan are ancestors of the first commit touching include, src, runner, libretro or host/bios.[ch], and the reversed check fails"
    requirement: BIOS-01
    verification:
      - kind: other
        ref: "git merge-base --is-ancestor <scan-add> <first-code> exit 0 for firmware_scan.cmake, firmware-window.sh and host/bios_table.inc, reversed exit 1 for each; recorded under Order evidence"
        status: pass
    human_judgment: false
  - id: E5
    description: "The nofp lane lists abi.api_v14 in its exact inventory; that lane cannot configure on the agent's machine (AppleClang, no GCC) so it runs only on the hosted dispatch"
    requirement: BIOS-01
    verification:
      - kind: other
        ref: "grep -c '^abi\\.api_v14	abi,portable$' tests/inventory/nofp.txt prints 1; cmake --workflow --preset nofp exit 1 locally at configure (needs GCC); hosted nofp legs not run for these commits"
        status: unknown
    human_judgment: false
---

# Phase 10 Plan 03: Media struct BIOS fields and the version 14 pin Summary

`cogmaster_media` gains `bios` and `bios_size` (with a 64-bit-only pad `reserved3`) after `fm`, the three versions move to API 14, behaviour 16 and state 16, `tests/api/v14.c` pins the new layout on three lanes, and the first code commit exists with both firmware scans proven to precede it. No source file reads the new fields yet.

## Performance

- **Duration:** about 25 minutes
- **Completed:** 2026-11-11
- **Tasks:** 2
- **Files:** 1 created, 9 modified

## Accomplishments

- `include/cogmaster.h` (9,951 bytes of 24,576): three version macros raised; after `fm`, `uint32_t reserved3` under the header's existing `UINTPTR_MAX > UINT32_MAX` pattern, `const void *bios` and `size_t bios_size`, with one short comment. No function, struct, constant or status added.
- `src/internal.h`: the media size assert became 72 / 40 and asserts for `bios` (56 / 32), `bios_size` (64 / 36) and `reserved3` (52, 64-bit only) were added. Nothing else in `src/` changed.
- `tests/api/v9.c` and `tests/core/test_core.c`: only the `sizeof(cogmaster_media)` comparison became `>=` (same 56 / 32). `tests/api/v13.c`: only the three macros and `v.api`, `v.behaviour`, `v.state` became `>=`; all 8 size and offset pins stay `==` (the count is unchanged from origin/main).
- `tests/api/v14.c` (new, 43 lines): versions `==` 14, 16, 16, ABI 1, media offsets and size, `reserved3` under the 64-bit condition, and the unchanged sizes of input, run result, info and cart info.
- `abi.api_v14` registered with `abi portable`, timeout 10 (the neighbours' value), on ci, wasm32 and nofp.

## Task Commits

1. Task 1, header delta, asserts, loosened pins, `v14.c`: `0d8a598`
2. Task 2, `abi.api_v14` and the three inventories: `4fa66ae`

## Order evidence

Commands from the repository root after `git fetch origin main` (exit 0) and with the first code commit being the Task 1 commit `0d8a598` (its subject is the last line of the list below).

```
git merge-base --is-ancestor <add of tests/cmake/firmware_scan.cmake> <first code commit>   exit 0
git merge-base --is-ancestor <first code commit> <add of tests/cmake/firmware_scan.cmake>   exit 1
git merge-base --is-ancestor <add of scripts/firmware-window.sh> <first code commit>        exit 0
git merge-base --is-ancestor <first code commit> <add of scripts/firmware-window.sh>        exit 1
git merge-base --is-ancestor <add of host/bios_table.inc> <first code commit>               exit 0
git merge-base --is-ancestor <first code commit> <add of host/bios_table.inc>               exit 1
```

The adding commits were found with `git log --diff-filter=A origin/main..HEAD -- <file>`: the release scan and `firmware_scan.cmake` came with "test(10-01): scan the release zips for known firmware digests and copier-headered members"; `host/bios_table.inc` with "test(10-01): check the BIOS digest table against the policy rows and ban BIOS directory reads in tests"; `firmware-window.sh` with "test(10-02): scan the tree, build outputs and zips for 32-byte windows of an owner dump". The first commit touching `include`, `src`, `runner`, `libretro` or `host/bios.[ch]` is "feat(10-03): append the firmware image fields to the media struct and raise the three versions". The reversed checks exit 1 for all three, so the comparison is not vacuous.

Ordered commit subjects of `git log --reverse --format=%s origin/main..HEAD` before this plan's own summary commit:

```
docs(10): record the phase context and its adversarial pass
docs(10): plan the BIOS boot phase
docs(10): revise the BIOS boot plans
test(10-01): check the BIOS digest table against the policy rows and ban BIOS directory reads in tests
test(10-01): scan the release zips for known firmware digests and copier-headered members
docs(10-01): summarise the digest table and release firmware scan plan
docs(10-01): record the plan position, metrics and decisions
test(10-02): scan the tree, build outputs and zips for 32-byte windows of an owner dump
test(10-02): stop retyped firmware bytes and run the window scan from the commit and push hooks
test(10-02): settle the firmware scan timeouts from the first hosted runs and give the hygiene job 3 minutes
docs(10-02): summarise the window scan, tripwire and hook plan and record the position
feat(10-03): append the firmware image fields to the media struct and raise the three versions
```

SHAs are not recorded as pins: the squash merge erases them.

## Evidence

All commands from the repository root; exit statuses as observed.

- `cmake --preset ci` and `cmake --build --preset ci` exit 0 after each task; `ctest --test-dir build/ci -R '^(abi\.|docs\.api_names|core\.)'` exit 0 (53 tests) after Task 1 with `v14.c` present but unregistered; `ctest -R '^abi\.api_version$'` exit 0 in that state.
- `grep -c 'define COGMASTER_API_VERSION 14'`, `...BEHAVIOUR_REVISION 16` and `...STATE_FORMAT_VERSION 16` on `include/cogmaster.h` each print 1; `wc -c < include/cogmaster.h` prints 9951.
- `git diff --name-only origin/main -- src` prints `src/internal.h` only; the added lines of `src/internal.h` are static asserts only; the planning-ID regex over the added lines of include, tests/api, tests/core and src counts 0.
- `clang-format --dry-run -Werror` over the six changed C files exit 0 (Homebrew `llvm@21`).
- Container compile `docker run --rm -v "$PWD":/src -w /src gcc:13 sh -c 'for f in tests/api/v9.c tests/api/v13.c tests/api/v14.c tests/core/test_core.c; do gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I include -I build/ci/generated -I src "$f" || exit 1; done'` exit 0.
- `ctest --test-dir build/ci -N -R '^abi\.api_v1[0-4]$'` prints `Total Tests: 5`; `grep -c '^abi\.api_v14	abi,portable$'` prints 1 for ci.txt, wasm32.txt and nofp.txt; `! grep -q abi.api_v14` over hygiene.txt, fuzz.txt, library.txt and vectors-full.txt exit 0.
- Negative control: a python edit of `COGMASTER_API_VERSION 14` to `15` (`git diff --stat HEAD -- include` showed 1 file changed, rebuild exit 0) made `ctest -R '^abi\.api_version$'` exit 8; after `git checkout -- include/cogmaster.h` and a rebuild it exited 0.
- `cmake --workflow --preset ci` exit 0 after each task; `cmake --workflow --preset hygiene` exit 0 (63 tests). The first hygiene run right after the ci workflow exit 8 because `hygiene.tree` hit its 60 s limit under load (59.95 s); `sh scripts/hygiene.sh --tree` exit 0, `ctest -R '^hygiene\.tree$'` passed alone in 22.6 s, and the hygiene workflow re-run alone exit 0.
- `cmake --workflow --preset nofp` exit 1 at configure on this machine ("nofp: -mgeneral-regs-only needs GCC, not AppleClang"); it needs the hosted lane (see E5).
- `git diff --quiet $(git merge-base origin/main HEAD) -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake` exit 0 and `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0.
- `git push origin phase/10-bios-boot` exit 0 (after the Task 2 commit).

## Hosted dispatch

None in this plan's scope. State at the end of the plan: `gh run list --workflow ci.yml` still shows the last four runs as failures (the three branch dispatches of plan 02 and main's dispatch, run https://github.com/szTheory/cogmaster/actions/runs/43053345486 at 15:56 UTC, failed at the apt step of perf); no newer main dispatch exists, so the runner archive-DNS fault is neither confirmed fixed nor re-observed, and from the agent's machine `azure.archive.ubuntu.com` and `archive.ubuntu.com` answer 200 (that says nothing about the runners). The workflow was not changed or worked around. The all-legs criterion (plan 02 D5) and the be32 and wasm32 timeouts of the plan 01 and plan 02 tests stay open: they need a dispatch that reaches those jobs, and the next planned one is after plan 05 (plan 05's dispatch).

## Budgets

`cmake -DROOT=$PWD -P tests/cmake/budgets.cmake`: exit 0, **833 counted check lines** / 9243 src lines (unchanged from plan 02: this plan adds no check code, only 6 header-assert src lines). The phase cap of 847 is unchanged, 14 lines of allowance remain.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The plan-implied preprocessor condition for the 64-bit-only assert does not compile**
- **Found during:** Task 1, writing the `reserved3` assert in `src/internal.h`
- **Issue:** `COGMASTER__P` is `sizeof(void *)`, which the preprocessor cannot evaluate in `#if`.
- **Fix:** the assert sits under `#if UINTPTR_MAX > UINT32_MAX`, the same condition as the header's pad.
- **Files modified:** src/internal.h
- **Commit:** 0d8a598

### Interpretations of the plan (no behaviour lost)

- `tests/api/v14.c` declares `const size_t p = sizeof(void *)` as `tests/api/v9.c` does, to pin both pointer widths with one expression.
- Plan 03's hosted check was not needed for its own claims (the header delta is inert); the nofp inventory line for `abi.api_v14` is untested hosted, as recorded in E5.

**Total deviations:** 1 auto-fixed, 0 architectural. **Impact:** none.

## Phase notes

- Plan 04 can name `media.bios` and `media.bios_size`; plan 05 is the first reader of the fields and the first behaviour change (the state format 16 and behaviour revision 16 take effect there).
- The pre-existing core test of the media struct's first-released size still passes with the struct at 72 / 40, because the load path accepts the earlier, shorter sizes (`FIRST_SIZE_MEDIA` is unchanged).

## Known Stubs

None.

## Threat Flags

None. The new pointer and length are ignored by the load path until plan 05; the trust-boundary handling (size checks of the image) lands with the code that reads them.

## Self-Check: PASSED

- tests/api/v14.c exists; include/cogmaster.h contains `bios_size`.
- Commits 0d8a598 and 4fa66ae are ancestors of HEAD (`git merge-base --is-ancestor` exit 0 for each).
