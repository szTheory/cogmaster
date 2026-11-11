---
phase: 10-bios-boot
plan: "09"
subsystem: runner
tags:
  - bios
  - runner
  - exit-status
  - cli
status: complete

requires:
  - 10-05
  - 10-07
  - 10-08
provides:
  - cogmaster-run --boot fast|bios and --bios FILE with a strict grammar, in any order, each at most once
  - exit 77 for an absent BIOS path, a missing --bios, a file that is not 8192 bytes and a Game Gear cartridge; exit 1 for any other read error; exit 2 for every misuse; fixed texts that never print the path or a byte
  - one stderr label line per BIOS run, recognised by SHA-1 and size for its name only, otherwise the file's own digest, and a correctly sized unrecognised dump still boots
  - runner.bios.grammar, runner.bios.refusals, runner.bios.no_path_echo and runner.bios.boot, each with a control, and one cross-lane device row for the built image
affects:
  - 10-10
  - 10-11

actuals:
  tokens: 21000
  tasks: 2
  commits: 2

plan_head_before: 181c0abbd221e045823f0a3765a84e7c6a5fb9fa
plan_head_after: 836fcd9
commits: 2

tech-stack:
  added: []
  patterns:
    - "A refusal class chosen by the cause: an absent file is refused (77) and any other read failure is a failure (1), as for the cartridge"
    - "Every BIOS message is a fixed text; the path-echo test runs each error under a directory named by a marker string, with a control (a missing cartridge) that does print its path today"
    - "The label line's digest is checked against CMake's own file(SHA1), an implementation independent of the core's"

key-files:
  created: []
  modified:
    - runner/args.c
    - runner/args.h
    - runner/main.c
    - runner/exit_status.h
    - docs/runner.md
    - tests/runner/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/cmake/hashes.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The Game Gear refusal and the absent-BIOS refusal are decided before the BIOS is read, and the absent BIOS (no --bios, or no file at the path) shares the one 'required and absent' text; the four fixed texts of the decision stay four"
  - "A file read at the 8193-byte cap is reported as over 8192 bytes, not as a size, because the read stops there; a recognised dump of a smaller size is named in the wrong-size text"
  - "The device row runs two frames, not ten: from the third frame the probe cartridge has the machine and a BIOS run hashes like a fast one, so the last-frame hash of a longer run would compare nothing the BIOS phase did"

requirements-completed:
  - BIOS-02

coverage:
  - id: R1
    description: "The runner accepts --boot fast|bios (default fast, fast equal to omission) and --bios FILE, each once, a value required and non-empty, in any order; every misuse exits 2 with a first line naming the option: an unknown, empty, missing or repeated --boot, a repeated or empty --bios, --bios without --boot bios (including --boot fast --bios F), --boot bios without a cartridge, --probe with either option and --expect with --boot bios; a missing BIOS and a missing cartridge are still exit 2 when the command line is misused, so nothing is read"
    requirement: BIOS-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.bios\\.grammar$' exit 0; controls in the same case: the valid spellings in either order exit 0 and --boot fast with --expect prints the execution line"
        status: pass
    human_judgment: false
  - id: R2
    description: "Exit 77 for no --bios, a --bios path that does not exist (also a path below a regular file on POSIX), a file of 0, 1, 8191, 8193 or 16384 bytes and a Game Gear cartridge by --machine gg or a .gg image under auto; exit 1 for a --bios directory and for a missing cartridge with a valid BIOS; the right owned file exits 0 and the Game Gear image runs without a BIOS"
    requirement: BIOS-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.bios\\.refusals$' exit 0; the absent path mapped to exit 1 made it exit non-zero ('expected exit 77') and the restore made it exit 0"
        status: pass
    human_judgment: false
  - id: R3
    description: "No BIOS error prints the path: each of absent, wrong size, directory, Game Gear and required-and-absent runs under a directory named NOECHO-MARKER-7f3a with the marker absent from standard output and error, a successful BIOS run prints the label without the marker, and the control (a missing cartridge under the marker) does print it, so the assertion can fail"
    requirement: BIOS-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.bios\\.no_path_echo$' exit 0; the 'cannot read the BIOS file' text changed to print the path made it exit non-zero ('the output holds the path marker') and the restore made it exit 0"
        status: pass
    human_judgment: false
  - id: R4
    description: "A BIOS run of the built image at frame 1, inside the BIOS phase, differs from the fast hash, repeats identically, --boot fast equals no flag, the label line carries a 40-hex digest equal to the SHA-1 CMake computes for the same file, a second owned 8192-byte file prints its own different digest, and the note holds no path"
    requirement: BIOS-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.bios\\.boot$' exit 0; K moved to 3 (after the hand-off) made it exit non-zero ('hash alike'); the runner hashing one byte fewer made it exit non-zero (the digest is not the file's SHA-1); each restore made it exit 0"
        status: pass
    human_judgment: false
  - id: R5
    description: "One device row for the built image booted from the built BIOS adds one line to the cross-lane hash file and no other line moves; the eight-lane equality compares it at the next hosted run, and no audio row is added"
    requirement: BIOS-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.hashes_(file|compare)' exit 0; grep -c bios build/ci/hashes.txt prints 2 (the fixture line and the device line); git diff against the merge base removes one line of tests/cmake/hashes.cmake (the row list's last entry, re-emitted with a new one after it)"
        status: pass
    human_judgment: false
  - id: R6
    description: "runner.exit.table keeps --help, runner/exit_status.h and docs/runner.md equal: the refused meaning gains 'or the BIOS asked for is absent or not supported' and the failure meaning gains 'a missing --bios path is refused (77), not a failure; a missing cartridge is still 1'; --help prints both options; the runner guide carries the two option rows, the Refusals bullet and the sentence that frame N of a BIOS run is not frame N of a fast run"
    requirement: BIOS-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.(usage\\.unknown|exit\\.table|probe\\.)' exit 0 (6 tests); build/ci/cogmaster-run --help exit 0 and grep for --boot and --bios exit 0; docs/runner.md is 32,710 bytes of 38,912"
        status: pass
    human_judgment: false
  - id: R7
    description: "Fast boot, the execution line, the ledger key, --probe and every existing runner test, ledger row and hash line are untouched; the only existing lines edited are the unknown-option loop and the --help regex of one test"
    requirement: BIOS-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.' exit 0 (90 tests before the fixtures case, 91 with it); git diff --quiet against the merge base over the pinned test directories and the two fuzz harnesses exit 0; the two removed lines of tests/runner/tests.cmake are the foreach entry and the regex"
        status: pass
    human_judgment: false

duration: about 60 minutes
completed: 2026-11-11
---

# Phase 10 Plan 09: The runner boots from a named BIOS Summary

`cogmaster-run --boot bios --bios FILE` now boots a Master System from the user's own 8,192-byte dump with a strict grammar, 77 for every absent or unsupported BIOS and 1 for a genuine read failure, one label line by SHA-1 that never prints the path or a byte, and fast boot, the ledger and every existing hash untouched.

## Accomplishments

- `runner/args.c`, `runner/args.h`: `--boot` (exactly `fast` or `bios`) and `--bios FILE`, each at most once, a non-empty value, order-free. `check_combination` gains the usage errors (`--bios` without `--boot bios`, `--boot bios` without a cartridge, `--expect` with `--boot bios`) and the two options join the `--probe` list. The synopsis prints both options.
- `runner/main.c`: `bios_load` runs after the cartridge is read (a missing cartridge is still exit 1) and only for `--boot bios`. It decides the Game Gear refusal first (`--machine gg`, or the probe's machine under auto), then reads through `cogmaster__bios_read` into an 8,193-byte stack buffer, maps absent to 77 and any other failure to 1, refuses a count other than 8,192 with the size (and the table name of a known smaller dump), identifies the file with the built-in rows and prints one stderr line, either `BIOS recognised: NAME` or `BIOS not recognised (sha1 DIGEST); running it as given`. `run` sets `media.bios` and `media.bios_size`. `load_status` carries a backstop so a profile refusal with a BIOS on a Game Gear says the BIOS text, never a Game Gear BIOS that is planned.
- `runner/exit_status.h` and `docs/runner.md`: the two exit meanings changed in all three places together; the guide gains the `--boot` and `--bios` rows, one Refusals bullet and the sentence that a BIOS run starts with the BIOS's own time so scripts written for fast boot misalign.
- `tests/runner/tests.cmake`: `runner.bios.grammar`, `runner.bios.refusals` and `runner.bios.no_path_echo` (script cases built from `expect_exit`, owned files written at test time with a size read-back that fails by name). The `unknown` case no longer lists `--boot` and `--bios` as unknown options.
- `tests/fixtures/tests.cmake`: `runner.bios.boot` on the built probe cartridge and the built BIOS-shaped image, with the independent `file(SHA1)` comparison and a second owned file. `bios_probe.sms|bios|--boot bios --bios ...` is the one device row in `tests/cmake/hashes.cmake`, fed by a two-line `bios.input` made like the phaser's.

## Task Commits

1. Task 1, grammar, load wiring, exit meanings, runner guide and the three tests: `19cd610`
2. Task 2, `runner.bios.boot` and the device row: `836fcd9`

## Evidence

All commands from the repository root; exit statuses as observed. No hosted dispatch in this plan (the next is after plan 10).

- `cmake --preset ci` exit 0 and `cmake --build --preset ci` exit 0 after each task; `ctest --test-dir build/ci -R '^runner\.' --output-on-failure -j4` exit 0 (90 tests after task 1, 91 with `runner.bios.boot`); `ctest --test-dir build/ci -N -R '^runner\.bios\.'` exit 0 printing `Total Tests: 3` after task 1 and `Total Tests: 4` after task 2.
- `ctest --test-dir build/ci -R '^runner\.(usage\.unknown|exit\.table|probe\.)'` exit 0 (6 tests); `ctest --test-dir build/ci -R '^(runner\.hashes_file|runner\.hash)'` exit 0 (9 tests); `ctest --test-dir build/ci -R '^runner\.(hashes_file|hashes_compare)'` exit 0 (3 tests).
- `build/ci/cogmaster-run --help` exit 0 and `grep -Eq -- '--boot'` and `grep -Eq -- '--bios'` over its output exit 0; `wc -c < docs/runner.md` prints 32710; the planning-identifier grep over the added runner-guide lines prints 0.
- Removed lines against the merge base: `tests/runner/tests.cmake` has exactly two (`foreach(opt --capabilities --boot --bios)` and the `--(capabilities|boot|bios)` regex line), none mentions `probe`; `tests/cmake/hashes.cmake` has exactly one (the last device-row entry, re-emitted with the new row after it).
- Negative controls, each a python edit, a successful rebuild (exit 0) and a red result, then restored and re-run green: (1) the 'cannot read the BIOS file' text printing the path, `runner.bios.no_path_echo` exit non-zero ('the output holds the path marker'); (2) the absent path mapped to exit 1 instead of 77, `runner.bios.refusals` exit non-zero ('expected exit 77'); (3) K moved to 3, after the hand-off, `runner.bios.boot` exit non-zero ('the BIOS run and the fast run hash alike at frame 3'); (4) identify hashing one byte fewer, `runner.bios.boot` exit non-zero (the printed digest is not the file's SHA-1). `git diff --stat HEAD` could not show the edits of (1), (2) and (4) as new changes (the file was already modified relative to HEAD during task 1) so each is tied to its build exit 0 and its red test.
- `/opt/homebrew/opt/llvm@21/bin/clang-format --dry-run --Werror runner/args.c runner/args.h runner/main.c runner/exit_status.h` exit 0.
- `docker run --rm -v "$PWD":/w -w /w gcc:13 sh -c 'for f in runner/args.c runner/main.c; do gcc -std=c17 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -Wsign-conversion -fsyntax-only -I runner -I host -I src -I include -I build/ci/generated $f || exit 1; done'` exit 0.
- `cmake --workflow --preset ci` exit 0 (621 tests and the 5 of the second stage) and `cmake --workflow --preset hygiene` exit 0 (63 tests) after each task.
- Unchanged proof: `git diff --quiet $(git merge-base origin/main HEAD) -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake tests/fuzz/fuzz_run.c tests/fuzz/fuzz_loader.c` exit 0; `! git diff $(git merge-base origin/main HEAD) -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt | grep -q '^-[^-]'` exit 0.
- No real firmware byte was read, copied, logged or compared. Recognition of a known dump is proved by `host.bios` of plan 08 through the real identify path with an injected table, because no real dump may exist in the tree; what these runner tests can fail on is the printed digest being the file's own SHA-1 (CMake's `file(SHA1)`) for two different owned files.

## Budgets

`cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: counted check code 833 (phase cap 847), src lines 9,399. No counted check code and no src line was added: the runner, the host helper and the test registration files are outside the count. `docs/runner.md` is 32,710 bytes of the 38,912 cap.

## Deviations from Plan

### Interpretations and measured corrections (no behaviour lost)

- **The device row runs two frames, not ten.** The plan asks for enough `.input` lines to cover the BIOS phase and the hand-off. Measured: frames 1 and 2 of the BIOS run show the image's backdrop (hash differs from the fast frame) and from frame 3 the probe cartridge has the machine and every BIOS and fast frame hashes alike, so the row's last-frame hash at ten frames equalled the fast boot's and would have compared nothing the BIOS phase did. The row's `bios.input` has two lines, so the cross-lane hash is the last BIOS-phase frame; the hand-off itself is the job of `bios.equivalence`.
- **Files of 0 and 1 byte are written whole.** `file(CONFIGURE ... CONTENT "")` wrote zero bytes (no final LF for an empty body), so a one-byte file by the 'size minus one plus LF' rule failed its own read-back. Sizes below 2 use `file(WRITE)` of `a` characters (no LF, so no CRLF risk); every other size is the size-minus-one body plus the LF that `file(CONFIGURE)` adds.
- **One text for both 'absent' causes.** No `--bios` and a `--bios` path that does not exist both print the 'required and absent' text of the decision, so the four fixed texts stay four; the exit class (77) is the same.
- **'over 8192 bytes' for a capped read.** The read stops at 8,193 bytes, so a longer file cannot be sized exactly; the text says `a BIOS file of over 8192 bytes` for any count at the cap and the byte count otherwise. A table name is added only for a known dump read in full (the table's 1,024-byte entry is the only one below the cap).
- **Game Gear decided before the BIOS argument is looked at.** The check needs only the cartridge, so a Game Gear cartridge with no `--bios` also reads as the Game Gear refusal; both are exit 77.
- **ENOTDIR case guarded.** The path-below-a-regular-file refusal is asserted off Windows, where the open reports a different error.

**Total deviations:** 0 rule-driven fixes; 6 interpretations recorded above. **Impact:** none on the requirement.

## Known Stubs

None.

## Threat Flags

None. The runner opens one more caller-named path read-only, reads at most 8,193 bytes into a stack buffer through the plan 08 helper, and every refusal text is fixed.

## Phase notes

- The README sentence about exit 77 lands with the libretro plan.
- The four `runner.bios.*` timeouts are an unsettled 20 s floor (marked provisional); plan 11 settles them from the dispatch after plan 10.
- `hygiene.tree` was not re-run alone: it passed inside `cmake --workflow --preset hygiene` each time.

## Self-Check: PASSED

- This SUMMARY exists; `runner/main.c`, `runner/args.c`, `runner/args.h`, `runner/exit_status.h`, `docs/runner.md` exist and are modified.
- Commits `19cd610` and `836fcd9` are ancestors of HEAD (`git merge-base --is-ancestor` exit 0 for each).
