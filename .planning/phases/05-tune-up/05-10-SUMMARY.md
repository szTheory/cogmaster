---
phase: 05-tune-up
plan: "10"
subsystem: docs
tags: [readme, play-a-game, docs-check, negative-control]
status: complete

requires:
  - phase: 05-tune-up
    provides: the docs split and budgets (plan 05-01), quality.json and the single docs cap (plan 05-05), size ceilings and the ratchet (plan 05-06)
provides:
  - a README "Play a game" section that leads with the no-BIOS sentence and gives RetroArch and runner steps
  - tests/cmake/readme.cmake, a check that runs the README's first runner command and holds its text true
  - docs.readme_play and docs.readme_play.self_test in the ci and wasm32 inventories
affects: [05-14]

actuals:
  tokens: 1400
  tasks: 2
  commits: 2

plan_head_before: a6176abe3a512d45682ff81681b6ab49d7a1bc5f
plan_head_after: 1d66a4633f488beaa59e8b7c9c1eaf9458bc489c
commits: 2

tech-stack:
  added: []
  patterns:
    - "A README command is executable documentation: the check swaps the placeholder file for a built fixture and compares the printed line with the README's text block, digest read as 64 hex digits"
    - "An extension rule reads only leading-dot backticked tokens, so file names such as game.sms are ignored"

key-files:
  created:
    - tests/cmake/readme.cmake
  modified:
    - README.md
    - tests/runner/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/policy/repository-checks.txt
    - docs/testing.md

key-decisions:
  - "The README check runs against the built boot_to_play.sms fixture, not a tests/roms/manifest.txt fixture"
  - "readme.cmake drops an input-presence loop: the CTest registration always passes the four inputs and a missing one fails the run anyway, which saves five counted check lines"

requirements-completed: [TUNE-06]

coverage:
  - id: R1
    description: "The README's Play a game section says how to load a ROM in RetroArch (Load Core, then Load Content) and with the runner, with the exit status 77 link"
    requirement: TUNE-06
    verification:
      - kind: other
        ref: "grep -q '^## Play a game' README.md && grep -q 'Load Content' README.md && grep -q 'runner.md#exit-status' README.md exit 0; the first bin/cogmaster-run line is the --frames 600 --hash game.sms command, awk and grep exit 0; the same grep exited 1 at HEAD before the edit"
        status: pass
    human_judgment: false
  - id: R2
    description: "The README states once that cogmaster ships no BIOS and no game and that a .sms or .gg file you own needs no BIOS, and stays under its byte budget"
    requirement: TUNE-06
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.readme_play' exit 0; README.md is 3637 bytes against 12288; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0"
        status: pass
    human_judgment: false
  - id: R3
    description: "A ci-preset test runs the README's first runner command against the built boot_to_play.sms and fails when the command, its printed line, the ROM extensions or the no-BIOS sentence drift"
    requirement: TUNE-06
    verification:
      - kind: integration
        ref: "tests/cmake/readme.cmake via docs.readme_play; RED run before the script existed failed both tests; GREEN run passed 2 of 2; cmake --workflow --preset ci exit 0 and cmake --workflow --preset hygiene exit 0"
        status: pass
    human_judgment: false
  - id: R4
    description: "The negative control, a README copy naming .sg where it names .gg, exits 1 with the message readme: .sg is not in supported_extensions, and a non-leading-dot token such as game.sms does not trip the extension rule"
    requirement: TUNE-06
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^docs\\.readme_play\\.self_test' exit 0; one-off cmake -DREADME=<README plus a game.sms and a zip file name token> -P tests/cmake/readme.cmake exit 0"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 10: README Play a game Summary

A README "Play a game" section leads with the no-BIOS sentence and gives RetroArch and runner steps, and a CMake `-P` check runs its first runner command against the built playable fixture so CI fails the day the text stops being true.

## Decisions

- The README check runs against the built `boot_to_play.sms` fixture (an owned image built from source, digest in
  tests/built/expected.txt) instead of a `tests/roms/manifest.txt` fixture as the CONTEXT README decision worded
  it. The built image is present on every lane that has COGMASTER_FIXTURES, be32 and wasm32 included, and adds no
  ROM bytes to the tree.
- The check is classified `check` in tests/policy/repository-checks.txt, as the CONTEXT budget bullet says.
- `docs.readme_play.self_test` passes the cross-compiling emulator as an escaped list so expect_fail.cmake sees
  one argument; this is the technique runner.hash.self_test already uses.

## Evidence

- Verify command (Task 1) exit 1 at HEAD before the edit, exit 0 after.
- RED: `ctest --test-dir build/ci -R '^docs\.readme_play'` failed both tests with no script; GREEN: exit 0, two tests.
- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0.
- Check-line ratio: `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0, 703 check lines of 745 allowed (7455 src
  lines), up from 667; readme.cmake counts 36 lines.
- Hosted run: none. A push of phase/05-tune-up triggers no CI run and the plan names no workflow dispatch, so the be32
  and wasm32 lanes that also register the two tests are first exercised by the phase pull request's CI run.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. The check budget is tight: 42 counted lines remain for the Dependabot policy and the ci_required actor clause.

## Self-Check: PASSED

- tests/cmake/readme.cmake exists; README.md holds the section; commits 571ae3b and 1d66a46 are ancestors of HEAD.
