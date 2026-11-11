---
phase: 09-light-phaser
plan: "08"
subsystem: policy-gun
tags:
  - policy
  - gun
  - tests
  - docs
status: complete

requires:
  - 09-04
  - 09-07
provides:
  - the no_title_table case of tests/cmake/repository.sh, a comment-stripping awk over the five files the sensor reaches
  - policy.no_title_table and three self-tests (array, hook, comments) on synthetic trees
  - ports.gun.identity, the owned fixture run with its header identity flipped on four profiles
  - a paragraph of docs/hardware/video.md on the sensor's early render
affects: [09-12]

actuals:
  tokens: 2250
  tasks: 3
  commits: 3

plan_head_before: 965319c172293ab375d67fa024d04afe6603a639
plan_head_after: 4384ebd88fdbff8d5cbfdaa47085833eccb1637d
commits: 3

tech-stack:
  added: []
  patterns:
    - "A repository check that strips comments with a small awk state machine before matching, so the hardware comments a module must carry cannot trip a rule about code"
    - "A self-test tree of five synthetic files per case, written by one CMake function, with one case that must pass so a rule cannot be satisfied by failing on everything"

key-files:
  created: []
  modified:
    - tests/cmake/repository.sh
    - tests/policy/tests.cmake
    - tests/ports/test_gun.c
    - tests/ports/tests.cmake
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - docs/hardware/video.md

key-decisions:
  - "The check scans gun.c, ports.c, io.c, vdp.c and vdp_render.c as code only; frame.c is not scanned because it tests the cartridge image for the test card, and the table's own home (cart.c, cartdb.c, instance.c) is outside it"
  - "ports.gun.identity flips $7FFA to $7FFE (checksum, product code, product and version nibble) and the region nibble of $7FFF, and compares the mailbox, the completed frame and the frame count, so a header-keyed branch on the owned image cannot pass"

requirements-completed:
  - GUN-01

coverage:
  - id: C1
    description: "A per-title table keyed on the loaded cartridge cannot enter the sensor or the hooks it reaches: code in gun.c, ports.c, io.c, vdp.c and vdp_render.c with a cart image use, a database, hash, title, product, serial or checksum word, or (in gun.c) an initialised array fails with its file and line, while the real source passes"
    requirement: GUN-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^policy\\.no_title_table$' exit 0 on the real src; one-shot python edits adding an initialised array to src/gun.c and a cart use to src/ports.c each turned it red naming the file and line, each reverted with git checkout and green again"
        status: pass
    human_judgment: false
  - id: C2
    description: "The check is proven to scan the hooks and not to trip on the mandatory hardware comments: the array on gun.c line 5 fails naming gun.c:5, a cart use on ports.c line 3 fails naming ports.c:3, and a tree whose five files mention cart, title, sha1 and crc32 only in comments (one spanning lines) passes"
    requirement: GUN-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^policy\\.no_title_table\\.self_test\\.(array|hook|comments)$' exit 0 (three tests); the same four names also run clean in a gcc:13 container with mawk"
        status: pass
    human_judgment: false
  - id: C3
    description: "The owned fixture's result does not depend on its header: with the product code, checksum and region nibble flipped in memory, the mailbox, the completed frame and the frame count equal the unflipped run on four profiles at a fixed aim, and a parked-aim run differs, so the equality compares a run that senses"
    requirement: GUN-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^ports\\.gun\\.identity$' exit 0; a one-shot python edit adding 16 ticks to the sensor delay when cart.rom byte $7FFF is $4C turned ports.gun.identity and policy.no_title_table red and was reverted with git checkout of src/gun.c"
        status: pass
    human_judgment: false
  - id: C4
    description: "The check code stays within its allowance: 12 counted lines added, 735 of a limit of 922 (9221 src lines), no new -P script, and the registrations are behaviour code"
    requirement: GUN-01
    verification:
      - kind: integration
        ref: "cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0 printing 735 check lines; ctest -R '^docs\\.budgets' exit 0; git diff --name-status origin/main -- tests/cmake scripts cmake lists no added file"
        status: pass
    human_judgment: false
  - id: C5
    description: "The renderer's treatment of the sensor is documented where the render model lives: dots strictly before tick / 2, none once the beam is past the last picture row, none while an unsupported mode is on, so the polling schedule cannot change a frame hash"
    requirement: GUN-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(docs\\.|policy\\.)' exit 0 (39 tests); the paragraph was read against src/gun.c run and cogmaster__vdp_render_until; docs/hardware/video.md is 14487 bytes against its cap of 38912"
        status: pass
    human_judgment: false
  - id: C6
    description: "Nothing else moved: both workflows pass and the existing latch, audio and test-card pins are byte-equal to origin/main"
    requirement: GUN-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0; cmake --workflow --preset hygiene exit 0; git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake exit 0; LC_ALL=C sort -c on both inventories exit 0"
        status: pass
    human_judgment: false

duration: 6min
completed: 2026-11-11
---

# Phase 9 Plan 8: The no-title-table check and the header-identity test Summary

**`policy.no_title_table` is a 12-line awk case in the existing repository dispatcher that strips comments and then fails on a cart use, a database or hash word, or an initialised array anywhere the Light Phaser sensor reaches, three self-tests prove it scans the hooks and spares the hardware comments, and `ports.gun.identity` shows the owned fixture's mailbox, frame and frame count do not move when its header is flipped on four profiles.**

## Performance

- **Duration:** 6 min by the clock of the commits (06:22 to 06:28)
- **Completed:** 2026-11-11
- **Tasks:** 3
- **Files:** 7

## Accomplishments

- `tests/cmake/repository.sh` gained the `no_title_table` case: a comment line naming the fault and a ten-line awk over the five files, `;;` on the last line, 12 counted lines. The awk keeps a block-comment flag across lines and strips `/* ... */` and `//` before it matches `(->|.)cart` not followed by an identifier character, the eight words, and in `gun.c` only `] = {`; each hit prints `no_title_table: <file>:<line>: <text>`. On the real `src` it exits 0 at this commit, so no regex was loosened.
- `tests/policy/tests.cmake` registers `policy.no_title_table` and, through one function that writes five files per case under `build/ci/policy/`, the self-tests `.array`, `.hook` and `.comments`. The first two go through `expect_fail.cmake` with the regexes `gun.c:5` and `ports.c:3`; the third is a plain run that must exit 0 on a tree whose comments mention cart, title, sha1 and crc32, one comment spanning two lines.
- `tests/ports/test_gun.c` has the `identity` case (image path, aim x and aim y on the command line). For each of four profiles it runs the fixture to its done flag (cap 600 frames) unchanged, with `$7FFA` to `$7FFE` inverted and the region nibble of `$7FFF` flipped in the test's own copy, and with the aim parked; it asserts equal mailbox bytes (`$C100` to `$C1FF`), equal completed frame and equal frame count between the first two, and a different mailbox for the parked run. The case's comment says it catches a header-keyed branch on the owned image only.
- `tests/ports/tests.cmake` registers `ports.gun.identity` (labels `ports portable`, 20 s) under `if(COGMASTER_FIXTURES)`; the aim is `SQ_X + 16`, `SQ_Y + 9` from the `.DEFINE`s of `light_phaser.asm`, parsed with `file(STRINGS)` and `math(EXPR)`, and the image is `${PROJECT_BINARY_DIR}/fixtures/light_phaser.sms`. The block's fault comment now names a delay or aim rule keyed on the loaded image's header.
- `docs/hardware/video.md` has one paragraph (10 added lines, nothing removed, 14487 bytes) on the sensor's early render, read against `run` in `src/gun.c` and `cogmaster__vdp_render_until`.

## Task Commits

1. **Task 1: policy.no_title_table with its three self-tests** - `30a1568` (test)
2. **Task 2: ports.gun.identity** - `3395966` (test)
3. **Task 3: the sensor's early render in docs/hardware/video.md** - `4384ebd` (docs)

**Plan metadata:** the commit that carries this file, `STATE.md` and `ROADMAP.md`.

## Files Created/Modified

- `tests/cmake/repository.sh` - the `no_title_table` case (12 lines)
- `tests/policy/tests.cmake` - four registrations and the case-writing function
- `tests/ports/test_gun.c`, `tests/ports/tests.cmake` - the identity case and its registration
- `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt` - five names each (four policy, one ports)
- `docs/hardware/video.md` - the early-render paragraph

## Decisions Made

- The check does not scan `frame.c` (it legitimately tests the cartridge image for the test-card branch) nor `cart.c`, `cartdb.c`, `instance.c`, as the plan says.
- The flipped bytes are the whole of `$7FFA` to `$7FFE` (checksum, product code, product and version nibbles) and the region nibble of `$7FFF` (`$4C` to `$3C`), so every header field a title key could use is changed at once.
- The identity case compares the completed frame structure, the stricter form of a frame hash, plus the frame count at which the done flag appeared.

## Evidence

All from the repository root, no `cd`, no committed log.

| Command | Exit |
| --- | --- |
| `cmake --preset ci` then `ctest --test-dir build/ci -R '^(policy\.no_title_table\|docs\.budgets)' --output-on-failure` (12 tests) | 0 |
| `ctest --test-dir build/ci -R '^(ports\.gun\.\|policy\.no_title_table)'` after `cmake --build --preset ci` | 0 |
| `ctest --test-dir build/ci -R '^(docs\.\|policy\.)'` (39 tests, 100 percent passed) | 0 |
| `cmake --workflow --preset ci` | 0 |
| `cmake --workflow --preset hygiene` (50 tests) | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` prints 735 check lines over 9221 src lines (723 before, 12 added, limit 922) | 0 |
| `git diff --numstat origin/main -- tests/cmake/repository.sh` shows 12 added | 0 |
| `git diff --name-status origin/main -- tests/cmake scripts cmake`, count of `^A` lines is 0 | 0 |
| `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` | 0 |
| `LC_ALL=C sort -c tests/inventory/ci.txt` and the same for `wasm32.txt` | 0 |
| `ctest -N -R '^ports\.gun\.'` counts 18 (17 before, plus `identity`); `ctest -N -R '^policy\.no_title_table'` counts 4 | 0 |
| `docker run ... gcc:13 gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only -I src -I include -I tests -I build/ci/generated tests/ports/test_gun.c` | 0 |
| `/opt/homebrew/opt/llvm@21/bin/clang-format --dry-run -Werror tests/ports/test_gun.c` | 0 |
| in the gcc:13 container (mawk) the check run on the three synthetic trees and on `src` | exits 1, 1, 0 and 0 |
| planning IDs in the added lines of `tests/cmake`, `tests/policy`, the other test files and `docs/hardware/video.md` | 0 |

One-shot controls (python edits, never sed; each confirmed with `git diff --stat HEAD -- src`, reverted with `git checkout -- <file>`, then green again):

| Edit | Result |
| --- | --- |
| a `static const unsigned char offs[] = {9, 19, 15};` appended to `src/gun.c` | `policy.no_title_table` failed, output `no_title_table: gun.c:201: ...`; reverted, exit 0 |
| a function using `m->cart.rom` appended to `src/ports.c` | failed naming `ports.c:180`; reverted, exit 0 |
| 16 ticks added to the sensor's delay when `m->cart.rom[0x7FFF]` is `$4C` in the edge call of `src/gun.c`; build exit 0 before the result was trusted | `ports.gun.identity` and `policy.no_title_table` both failed; reverted with `git checkout -- src/gun.c`, then `cmake --workflow --preset ci` exit 0 |

No hosted dispatch: the plan names none, and a policy check, a CMake registration and a paragraph need no hosted lane beyond the pull request's own run. The identity test uses only the public API calls and the file-reading calls other ports tests already use, so it is expected on every lane; it first meets the hosted lanes in the next dispatch of the phase.

## Deviations from Plan

### Clarifications

**1. [Clarification] The earlier ports timeout had already been settled**
- **Found during:** Task 2
- **Issue:** the plan asks to rewrite the `ports.state.gun` timeout from the 120 s provisional wording using dispatch 2. Plan 09-07 already did that (`ade085d`): the comment reads `20 s floor; 2 x 4.52 s (be32), slowest of 1 run, run 43010180943` and `grep -c '120 s floor; provisional'` prints 0. Settling it again would only repeat the edit, so none was made. The plan's other grep, `grep -B8 'ports\.state\.gun'`, matches nothing because the registration is built from `ports.state.${case}`; the substance (a measured comment with a run id on the `gun` branch) holds.
- **Fix:** none needed.
- **Commit:** `ade085d` (plan 09-07)

**2. [Clarification] The extended comment is the block's own**
- The block's "Fault prevented" comment was extended with one clause as asked; the identity registration has a short comment of its own naming what the case flips and what it cannot catch.

**Total deviations:** none auto-fixed, 2 clarifications. **Impact:** none on behaviour or on a requirement.

## Known Stubs

None.

## Threat Flags

None. The plan adds a repository check, test registrations and a paragraph of prose; no network, auth or file-access surface (the identity test reads one build-tree image read-only and writes nothing).

## Phase notes

- Check lines: 735 of a limit of 922 (about 187 spare); the phase's allowance of at most 737 holds with 2 lines unused.
- `ports.gun.identity` stays at its provisional 20 s floor, with the comment `Timeout: 20 s floor; provisional.`; the closing plan reads the final hosted run and settles it. It runs in 0.09 s locally.
- The origin of the five files in the check is a fixed list in the awk call; a sixth file the sensor comes to reach has to be added there on purpose.

## Self-Check: PASSED

- FOUND: tests/cmake/repository.sh, tests/policy/tests.cmake, tests/ports/test_gun.c, tests/ports/tests.cmake, docs/hardware/video.md
- FOUND commits 30a1568, 3395966 and 4384ebd (`git merge-base --is-ancestor` each)
