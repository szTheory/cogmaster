---
phase: 11-pal-and-japanese-fm
plan: "01"
subsystem: foundations
tags:
  - docs
  - hygiene
  - ci-speed
  - provenance
status: complete

requires: []
provides:
  - docs/testing.md has 5,579 bytes of room (31,339 of 38,912) because the controller test table and the sound-hash prose moved to the files that own them
  - PROVENANCE section 2 names MAME paths that answer 200 at the pin
  - docs/maintaining.md CI speed section with measured local numbers
  - scripts/hygiene.sh reads each file once; hygiene.tree.header_offset keeps the content scan honest
  - hygiene.analyzer runs four clang-tidy processes at a time over the same files and checks
affects:
  - 11-02
  - 11-03
  - 11-09

actuals:
  tokens: 11000
  tasks: 3
  commits: 3

plan_head_before: 77fb0b1b8add9a9165a1fac4c76df39713574cb7
plan_head_after: 34dc616a55af5373b570ab0abdc3d04be50da517
commits: 3

tech-stack:
  added: []
  patterns:
    - "Read once into one mktemp file removed by an EXIT trap, then run every later step from that file"
    - "Parallel analyzer through printf into xargs -n 9 -P 4; a failing process makes xargs exit non-zero"

key-files:
  created: []
  modified:
    - docs/testing.md
    - docs/controllers.md
    - docs/audio.md
    - docs/maintaining.md
    - PROVENANCE.md
    - scripts/hygiene.sh
    - tests/hygiene/tests.cmake
    - tests/inventory/hygiene.txt

key-decisions:
  - "The moved blocks are cut and pasted whole; only the heading lines, one lead-in sentence in audio.md, one pointer sentence each in testing.md and one self-link in controllers.md changed"
  - "The CI speed section was written in two steps (the rules in the docs commit, the measured numbers in the analyzer commit) so that no commit holds a placeholder"
  - "The EXIT trap also carries a HUP INT TERM handler that exits 1, so an interrupted scan removes its temporary file too"

requirements-completed: []

coverage:
  - id: C1
    description: "The controller test table and the sound-row paragraphs moved, not copied, to docs/controllers.md and docs/audio.md; no anchor to a moved heading survives; the four docs stay under the cap with room"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R docs. and policy. exit 0 (57 tests); wc -c bounds on the four docs exit 0; grep for the old anchors exits 1"
        status: pass
    human_judgment: false
  - id: C2
    description: "The PROVENANCE MAME row names ymopl.cpp and ymopl.h and 3rdparty/ymfm/src/ymfm_opl.* instead of the path that answers 404, keeps its reference and never cites ym2413.cpp"
    requirement: REG-01
    verification:
      - kind: other
        ref: "gh api -X GET repos/mamedev/mame/contents/src/devices/sound/ymopl.cpp, ymopl.h and 3rdparty/ymfm/src/ymfm_opl.cpp with ref set to the full pin each exit 0"
        status: pass
    human_judgment: false
  - id: C3
    description: "docs/maintaining.md CI speed section (644 bytes) states the critical path, the macOS rule, the rerun for a runner DNS failure and the measured local numbers; docs/testing.md links it"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "awk section size and wc -c bounds exit 0; ctest -R docs. exit 0"
        status: pass
    human_judgment: false
  - id: C4
    description: "check_file reads each file once into one temporary file removed on every exit path; findings, texts and order unchanged; counted check lines of hygiene.sh unchanged"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/hygiene -R hygiene.tree, hygiene.byte_run and policy.firmware_window exit 0 (25 tests); cmake --workflow --preset hygiene exit 0; budgets.cmake prints 845 check lines"
        status: pass
    human_judgment: false
  - id: C5
    description: "hygiene.tree.header_offset: a file with the Sega header marker at byte 32,752 and no ROM-like name is named by content; removing the offset loop turns the test red"
    requirement: REG-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/hygiene -R hygiene.tree.header_offset exit 0; with the loop removed exit 8; restored exit 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "hygiene.analyzer runs the same checks over the same 29 files four processes at a time; a planted null dereference still fails through the xargs form"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/hygiene -R hygiene.analyzer exit 0; planted-file control through the xargs form exit 1; clean xargs run over src exit 0"
        status: pass
    human_judgment: false
  - id: C7
    description: "No file under include or src changed; the counted check code is unchanged at 845 and every existing test of the hygiene and ci presets is green"
    requirement: REG-01
    verification:
      - kind: integration
        ref: "git diff --quiet origin/main -- include src exit 0; cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exit 0; cmake --workflow --preset hygiene exit 0; cmake --workflow --preset ci exit 0"
        status: pass
    human_judgment: false

duration: about 50 minutes
completed: 2026-11-11
---

# Phase 11 Plan 01: Foundations, room, speed, provenance Summary

The controller and sound-hash prose now live with their topics (testing.md fell from 36,918 to 31,339 bytes), the MAME provenance row names paths that exist at the pin, the tree scan reads each file once with a new header-at-offset control, the static analyzer runs four processes at a time, and the CI speed note carries the measured local numbers. No core file changed.

## Accomplishments

- Docs moves: the whole "Controller tests" section of docs/testing.md is now "## Tests" in docs/controllers.md (before "## Limits"); the sound-row paragraphs with the two `grep` regeneration lines are now "## The frozen sound hashes" in docs/audio.md (after "## Reset"). testing.md keeps one sentence for each and the unchanged hash-equality command block.
- PROVENANCE.md section 2: the MAME row names `src/devices/sound/ymopl.cpp` and `ymopl.h` and `3rdparty/ymfm/src/ymfm_opl.*` (BSD-3, at the pin); the ymfm row and its reference are untouched; no row, notice or `ym2413` mention added.
- docs/maintaining.md "## CI speed" (644 bytes) after "## Weekly digest"; docs/testing.md's CI section links it.
- scripts/hygiene.sh: one `mktemp` file and one EXIT trap at the top; `check_file` reads its input once and every later step (header offsets by `dd if=`, emptiness by `-s`, binary test by `grep -Iq`, `check_text` on standard input) works from that file. 95 lines before and after.
- tests/hygiene/tests.cmake: `hygiene.tree.header_offset` (throwaway repository, copy of hygiene.sh, untracked `data.txt` of 32,768 NUL bytes with `TMR SEGA` at offset 32,752 written with `dd` and `printf`; expects exit 1 and `hygiene: game image by content`); `hygiene.analyzer` command through `printf '%s\n' src/*.c | xargs -n 9 -P 4`. tests/inventory/hygiene.txt gains one line.

## Task Commits

1. Task 1, docs moves, provenance, CI speed rules: `3a9b485`
2. Task 2, read-once tree scan and the header-offset control: `d8334c4`
3. Task 3, parallel analyzer and the measured numbers: `34dc616`

## Evidence

All commands from the repository root, in this working tree, on this Mac (18 cores). No hosted run belongs to this plan.

Docs bytes (wc -c, before then after): testing.md 36,918 then 31,339 (fell 5,579, bound at most 33,500); controllers.md 17,285 then 21,358 (at most 21,900); audio.md 14,478 then 16,416 (at most 16,600); maintaining.md 19,017 then 19,661 (at most 19,717 after this plan; the CI speed section is 644 bytes of at most 700). embedding.md untouched.

Checks:

- `cmake --preset ci` then `ctest --test-dir build/ci -R '^(docs\.|policy\.)'`: exit 0, 57 of 57 passed.
- `grep -rn -e '#controller-tests' -e '#comparing-hashes-across-lanes' README.md docs`: exit 1 (no match). Planning-identifier grep over added docs lines (`git diff -U0 origin/main -- docs`): prints 0.
- `gh api -X GET repos/mamedev/mame/contents/src/devices/sound/ymopl.cpp -f ref=1d7a454ed79e97d35d2a9f127ac029ead94711d6 --jq .path`: exit 0 (also `ymopl.h`: exit 0). Same for `3rdparty/ymfm/src/ymfm_opl.cpp`: exit 0. `grep -c 'ymfm/\*' PROVENANCE.md` prints 0, `grep -c ym2413` prints 0, `grep -c PV.03` prints 2.
- `sh scripts/hygiene.sh --tree` baseline on the unchanged script, three runs, each exit 0: 29, 27 and 26 s, median 27 s. After the rewrite, three runs, each exit 0: 18, 21 and 23 s, median 21 s (`test 21 -lt 27` exit 0). The runs are wall seconds from `date +%s` and noisy; the gain is the removed rereads.
- Mutation control: a python edit removing the offset loop from `check_file` (`git diff --stat scripts/hygiene.sh` showed 4 insertions, 8 deletions), `cmake --preset hygiene`, `ctest --test-dir build/hygiene -R '^hygiene\.tree\.header_offset$'` exit 8 (Failed); script restored from the saved copy (`cmp` exit 0), configure again, same ctest exit 0.
- Temporary file: with `TMPDIR` set to an empty private directory, `sh scripts/hygiene.sh --tree`, `--bogus` (exit 2) and `--staged` each left the directory empty (`ls -A | wc -l` prints 0).
- `ctest --test-dir build/hygiene -N -R '^hygiene\.tree'`: Total Tests 5. `git diff --numstat origin/main -- tests/inventory/hygiene.txt` prints `1 0`.
- `ctest --test-dir build/hygiene -R '^(hygiene\.tree|hygiene\.byte_run|policy\.firmware_window)'`: exit 0, 25 of 25 passed. The new control ran in 0.05 s (timeout comment `0.06 s slowest local run`, provisional, 20 s floor).
- `hygiene.analyzer` before, the unchanged serial command, three runs of `ctest --test-dir build/hygiene -R '^hygiene\.analyzer$'` (exit 0 each): 17, 21 and 23 s, median 21 s. After, three runs (exit 0 each): 15, 16 and 17 s, median 16 s. A later direct check of the serial form on the same busier machine took 28, 25 and 27 s, so the parallel form is faster in both comparisons.
- Planted control (a throwaway C file under the session scratch directory with `int *p = 0; return *p;`, never in the tree), same `-checks` string, `--quiet`, `--warnings-as-errors='*'`: direct run with `-- -std=c17 -I include -I src` before the edit exit 1 with `Dereference of null pointer`; the xargs form over the planted file plus `src/*.c` after the edit exit 1 (BSD xargs here; GNU xargs on the Linux runners exits 123) and prints the same finding twice; the xargs form over `src/*.c` only exit 0. `printf '%s\n' src/*.c` lists 29 files and equals `ls src/*.c` (`cmp` exit 0), so the file set is unchanged.
- `grep -c 'xargs -n 9 -P 4' tests/hygiene/tests.cmake` prints 1; `warnings-as-errors` count 1 here and at origin/main.
- `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0: 845 check lines (baseline 845, cap 848) and 9,402 src lines.
- `git diff --quiet origin/main -- include src` exit 0.
- `cmake --workflow --preset hygiene` exit 0 (71 tests, run before the last commit and again after the analyzer change) and `cmake --workflow --preset ci` exit 0 on the head.

## Deviations from Plan

### Interpretations (no behaviour lost)

- **File count of the analyzer.** The plan says 36 files; `src/*.c` holds 29. The command, checks and file set are unchanged; the figure in the plan was an estimate.
- **xargs exit status.** The plan expects 123 on a failing run. macOS (BSD) xargs exits 1; GNU xargs on the hosted Linux runners exits 123. The plan allows "or non-zero"; the control records exit 1 with the finding printed. The comment above the registration says "non-zero (123 with GNU xargs)".
- **The planted control runs the xargs form by hand.** The registered test cannot hold a planted file (nothing may be planted under `src/` or `tests/`), so the control is the same command shape over a file list holding the planted file, as the plan describes.
- **`cmake --workflow --preset ci` ran once, after task 3**, not after each task: tasks 1 and 2 touch only docs and the hygiene preset, which the `ci` preset does not run (docs.* and policy.* ran after task 1; the hygiene workflow ran after tasks 2 and 3).
- **No fault-prevented comment in hygiene.sh.** The plan puts the fault comment above the new registration; a comment line in the script would have added a counted line, so the script gained none (95 lines before and after) and the file header's existing fault line stands.
- **Signal traps.** The trap line also installs `trap 'exit 1' HUP INT TERM` so that an interrupted scan runs the EXIT trap; no extra line.

**Total deviations:** 0 rule-driven fixes; 6 interpretations above. **Impact:** none on the plan's outcome.

## Known Stubs

None.

## Threat Flags

None. The scan rewrite keeps every finding rule (controls above); no new endpoint, auth path or trust boundary.

## Notes for the next plans

- docs/testing.md has about 7.5 KB of room under the cap for the phase's later additions; controllers.md has 542 bytes under its plan bound and audio.md 184 bytes, so the sound-row additions of the audio plans go to testing.md or audio.md only within those figures.
- maintaining.md CI speed may grow by 306 bytes in the closing plan (cap 950 bytes for the section, 19,967 for the file; it is 19,661 now).

## Self-Check: PASSED

- Commits `3a9b485`, `d8334c4` and `34dc616` are ancestors of HEAD (`git rev-list --count 77fb0b1..HEAD` prints 3); each message ends with the attribution trailer.
- The edited files exist and `git diff --quiet origin/main -- include src` exits 0.
