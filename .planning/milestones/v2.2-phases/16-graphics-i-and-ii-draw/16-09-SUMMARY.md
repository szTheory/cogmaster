---
phase: 16-graphics-i-and-ii-draw
plan: 09
subsystem: docs
tags:
  - hardware-notes
  - provenance
  - legacy-video
  - timeouts
  - phase-head-evidence
  - third-push
  - hosted-run

requires:
  - phase: 16-graphics-i-and-ii-draw
    provides: the legacy renderer and sprite engine (plans 04 and 07), state format 17 and behaviour revision 19 (plan 06), the legacy fixtures and the hosted run 43562888383 whose durations size the timeouts (plans 05 and 08)
provides:
  - the legacy video hardware notes in docs/hardware/video.md, every choice said to be a model with its disagreeing sources
  - PROVENANCE rows for MAME tms9928a and ares tms9918 at two public pins, read never copied
  - SMS-VDP section 7 rewritten from sources with the MAME attribution corrected, VDP.21 re-pinned, rows VDP.27 and VDP.28 added
  - the embedding guide row for fault kind 6 and the revision 19 sentence, and the conformance guide's legacy fixture subsection, both inside the byte cap
  - runner.hashes_file at 130 s and the g2_late tests at 29 s, sized from run 43562888383, with that run id in the comments
  - the third push, its hosted run 43564813364 and the phase-head evidence for MODE-05 and MODE-07
affects:
  - ship

actuals:
  tokens: 4100
  tasks: 3
  commits: 3
plan_head_before: 13d8a27081486d4df51115e05809871a2944efd3
plan_head_after: 6e8984eda7945a15ce236f38bf41454f3286a71d

tech-stack:
  added: []
  patterns:
    - "a hardware note says model for each selected choice and lists the sources that disagree, so a fixture is read as fidelity to the notes and never as a measurement"
    - "one registration loop takes a per-variant timeout, so the one slow image (the late Graphics II image) gets twice its measured time and the fast images keep the floor"

key-files:
  created: []
  modified:
    - docs/hardware/video.md
    - PROVENANCE.md
    - docs/embedding.md
    - docs/conformance.md
    - .planning/preparation/SMS-VDP.md
    - tests/runner/tests.cmake
    - tests/fixtures/tests.cmake

key-decisions:
  - "The TI manual's sprite range is not quoted: the page image was not checked, so the docs say the range fits above $E0 and $E0 or more alike"
  - "The Young notes archive capture is 20260307023015, read from the redirect of a curl -sI on the web/2026/ form because the research did not record a timestamp; it answers 200"
  - "The three g2_late tests share one timeout of 29 s, twice the slowest of the three (g2_late.sms2, 14.02 s on be32), the grouping the existing comments use"
  - "core.state.legacy_sprite_bounds keeps its 60 s tier: 2 x 9.06 s is under it, and tests/core/tests.cmake is not in this plan's file list"

requirements-completed:
  - MODE-02
  - MODE-04
  - MODE-05
  - MODE-07
  - MODE-08

coverage:
  - id: D1
    description: "Graphics II masks are written up in docs/hardware/video.md and SMS-VDP section 7 with the choice, its reason and the disagreeing sources (msvdp, MAME 315_5124.cpp, ares, the TMS9918A rule). D-22 meets the success criterion's fixture per mask combination: the eleven (R3, R4) pairs run in process under the closed-form oracle (vdp.legacy.masks) plus the two ROM images through cogmaster-run (legacy_modes_g2 with R3 $FF and R4 $03, legacy_modes_g2_masked with R3 $9F and R4 $00), each on the Master System 1, the Master System 2 and the Game Gear in Master System video mode"
    requirement: MODE-02
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.legacy.masks, inside cmake --workflow --preset ci exit 0 (737 of 737)"
        status: pass
      - kind: integration
        ref: "fixtures.legacy_modes.g2 and fixtures.legacy_modes.g2_masked on three profiles pass in the same workflow; hosted CI run 43564813364 on 6e8984e concluded success"
        status: pass
      - kind: other
        ref: "grep -c tms9918a docs/hardware/video.md prints 2; grep -ci model docs/hardware/video.md prints 14; the Young row of SMS-VDP matches the capture-URL and access-date regex once"
        status: pass
    human_judgment: false
  - id: D2
    description: "The documentation says which modes render and which fault: video.md names nibbles 0 and 2 as rendering on the three profiles and lists text, multicolour, the mixed modes, the invalid text modes and native Game Gear video as faulting with the nibble in the fault address; embedding.md row 6 and the revision 19 sentence agree. The per-mode fault tests and the Light Phaser agreement are the earlier plans' tests and passed in the same workflow"
    requirement: MODE-04
    verification:
      - kind: unit
        ref: "ctest --preset ci -R vdp.unsupported and vdp.background.unsupported pass inside cmake --workflow --preset ci exit 0"
        status: pass
      - kind: other
        ref: "ctest --preset ci -R docs. and policy. exit 0 (68 tests); grep -c 'Behaviour revision 19' docs/embedding.md prints 1 and the old kind 6 wording prints 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "The notes are rewritten from sources, the MAME attribution is corrected in the author's words (315_5124.cpp masks on every revision, the no-mask statement is msvdp's, tms9928a.cpp uses above $E0), PROVENANCE section 2, 4 and 10 name MAME tms9928a.cpp and .h and ares component/video/tms9918 at pins that answer 200, VDP.21 is re-pinned, VDP.27 (ares) and VDP.28 (Young, tms9918a.txt, capture 20260307023015, licence not stated) are added, and the docs stay under 38,912 bytes (embedding 37,204, conformance 38,034)"
    requirement: MODE-08
    verification:
      - kind: other
        ref: "gh api repos/mamedev/mame/commits/1d7a454ed79e --jq .sha exit 0; gh api repos/ares-emulator/ares/commits/9408cb43d494 --jq .sha exit 0"
        status: pass
      - kind: other
        ref: "cmake -DROOT=. -P tests/cmake/budgets.cmake exit 0 (887 check lines, 9738 src lines); ctest --preset ci -R docs., policy. and release.single_version_source exit 0; test of wc -c below 38912 for both docs exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "No shipped frame, audio or state pin moved, behaviour revision 19 and state format 17 are recorded, a legacy state round trip is equal and the loader refuses legacy slots that break the bounds, on the phase head"
    requirement: MODE-05
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci exit 0 (737 of 737, including core.state.legacy_roundtrip and core.state.legacy_sprite_bounds); test -z of LC_ALL=C comm -23 of the sorted build/p16-base-hashes.txt and build/ci/hashes.txt exit 0 (173 base lines, 198 head lines)"
        status: pass
      - kind: integration
        ref: "hosted CI run 43564813364 job hash-equality success: the eight lanes' hashes files equal byte for byte"
        status: pass
    human_judgment: false
  - id: D5
    description: "The legacy renderer is selected once per call and the instructions-per-frame gate is green with no allow line, read from the hosted run of the pushed head: CI required success, perf success with boot_to_play 10089912 against a base of 12288947, tests/perf/allow.txt, ci.yml and packaging.cmake unchanged against main"
    requirement: MODE-07
    verification:
      - kind: other
        ref: "git push origin phase/16-graphics-i-and-ii-draw exit 0 (fba1e24 to 6e8984e); gh run list headSha equals git rev-parse HEAD; gh run view 43564813364 CI required job conclusion success; perf boot_to_play head=10089912 base=12288947 delta_bp=-1789 verdict=PASS"
        status: pass
      - kind: other
        ref: "git diff --quiet origin/main -- tests/perf/allow.txt exit 0; git diff --name-only origin/main -- .github/workflows/ci.yml cmake/packaging.cmake tests/perf/allow.txt prints nothing"
        status: pass
    human_judgment: false
  - id: D6
    description: "The runner.hashes_file timeout is 130 s from 2 x 64.72 s on the be32 lane and the three g2_late tests are 29 s from 2 x 14.02 s, both from run 43562888383 with that run id in their comments; the other legacy fixture tests keep the 10 s floor with their measured times. The head run measured 65.47 s and 13.81 s, inside both"
    verification:
      - kind: other
        ref: "sh scripts/ci-durations.sh tests 43562888383 and 43564813364 (the slowest seconds per test, exit 0); the full ci workflow passes with the new values, exit 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "The hygiene workflow, the tree scan, the format dry run and the budgets pass on the head"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene exit 0 (84 of 84); sh scripts/hygiene.sh --tree exit 0; clang-format 21 dry run over git ls-files exit 0; cmake -DROOT=. -P tests/cmake/budgets.cmake exit 0"
        status: pass
    human_judgment: false
  - id: D8
    description: "The agent read video.md, embedding.md and conformance.md against src/vdp_legacy.c and cogmaster__vdp_class in src/internal.h: the nibbles that render and fault, the 192-line and no-scroll model, the Graphics I and II address formulas, the mask expressions, the palette table and the CRAM 16 + n rule, the display-gated backdrop, the sprite table and pattern addresses, the quadrant order, plain doubling, the $E0 threshold, the early clock, the fifth-sprite index and the $1F read, the coincidence rule and the lines from 192 on, and the fixture counts against tests/fixtures/tests.cmake and the asm headers. One wording slip (an ambiguous clause about Young and MAME's tms9928a.cpp) was fixed before the commit"
    verification:
      - kind: other
        ref: "the agent's own read, rule 6; each claim traced to a line of src/vdp_legacy.c, src/vdp_render.c, src/internal.h, tests/fixtures/tests.cmake or the fixture assembly headers"
        status: pass
    human_judgment: false

duration: 21 min
completed: 2026-11-14
status: complete
---

# Phase 16 Plan 09: Hardware notes, provenance, timeouts from the hosted run and the third push Summary

**The legacy video notes say which modes render and which fault and call every selected choice a model with its disagreeing sources, the MAME attribution is corrected from a read of the BSD-3 file, the provenance file names the permissive files consulted at pins that answer 200, the hashes-file and late-image timeouts are twice the slowest hosted lane, and the third push has a green hosted run with the perf gate PASS and no allow line**

## Performance

- **Duration:** 21 min
- **Started:** 2026-11-14T05:57:15Z
- **Completed:** 2026-11-14T06:18:04Z
- **Tasks:** 3 (the first a tracer)
- **Files changed:** 7 (139 insertions, 29 deletions), none under `src/`, `include/` or `runner/`

## Accomplishments

- **The hardware notes.** The legacy paragraph of `docs/hardware/video.md` (84 lines now where five stood) says Graphics I (nibble 0000) and Graphics II (0010) render on the Master System 1, the Master System 2 and the Game Gear in Master System video mode; lists what still faults (text 0001, multicolour 0100, the mixed modes 0011, 0101, 0110 and 0111, the invalid text modes 1001 and 1101 everywhere and 1011, 1110 and 1111 on the 315-5124, and both legacy nibbles on native Game Gear video) with the nibble in the fault address; and writes modes as the nibble or as R0 and R1 bits. It states the 192-line, 32 by 24, no-scroll, no-left-mask model, the Graphics I and II address formulas, the Graphics II mask model with its three-way source disagreement and the all-ones agreement, the palette and the CRAM 16 + n rule, the display-gated backdrop deviation (bounded and pin-safe), and a legacy sprite paragraph with each model's disagreeing sources. `tms9918a.txt` is named beside the preparation section as the oracle's source for the unmasked addresses. The word model appears on 14 lines and no fixture is described as proving silicon.
- **The attribution correction.** `.planning/preparation/SMS-VDP.md` section 7 is rewritten from the sources: `315_5124.cpp` masks Graphics II on every revision and has one routine for all of them, so the SMS2 and Game Gear no-mask statement is msvdp's alone; `tms9928a.cpp` tests above $E0 and couples R3 into the pattern index on the 99 family; ares applies R4 bits to both tables. The R0 bit 3 line is replaced (attribute bit 7 is the early clock, R0 bit 3 is ignored, msvdp disagrees), the TI manual is recorded as having a usable text layer (VDP.26), VDP.21 is pinned to `1d7a454ed79e`, and the open question on TMS modes is replaced by the settled choices and what would reopen them (the SG-1000 machine, a hardware capture). The Young notes row is VDP.28 (capture `https://web.archive.org/web/20260307023015/https://www.smspower.org/uploads/Development/tms9918a.txt`, accessed 2026-11-13, licence not stated, read for facts), cited where section 7 names the masks and the oracle's sources; the ares row is VDP.27.
- **Provenance.** `PROVENANCE.md` section 2 adds MAME `src/devices/video/tms9928a.cpp/.h` (BSD-3, header line 1, headers read at the pin) and extends the ares ISC scope to `ares/component/video/tms9918/**`; section 4's read-never-copied list names both and says the legacy renderer is own code with no seed row; section 10's PV.01 and PV.02 rows name the files. Both pins answered 200: `gh api repos/mamedev/mame/commits/1d7a454ed79e --jq .sha` exit 0 (`1d7a454ed79e97d3...`), `gh api repos/ares-emulator/ares/commits/9408cb43d494 --jq .sha` exit 0 (`9408cb43d4948fc3...`). No GPL or LGPL source was opened; only the first three lines of the two BSD-3 MAME files were read to name the holders.
- **Guides.** `docs/embedding.md` kind 6 reads "an enabled display selects text, multicolour, a mixed mode, an invalid text mode, or a legacy mode on native Game Gear video" and gains one "Behaviour revision 19" sentence (37,204 bytes of 38,912). `docs/conformance.md` gains a "The legacy video fixtures" subsection: the five legacy_modes images, the closed-form oracle, the switch line pinned by hash, and the sprite scenarios with their mailbox rows judged by derived rows (38,034 bytes).
- **Timeouts.** From the hosted run of the second push (43562888383, be32 lane for each): `runner.hashes_file` 130 s (2 x 64.72 s; was 107 s), the three `g2_late` tests 29 s (2 x 14.02 s; sms1 7.94 s and gg_sms 6.72 s share the group value), and the provisional comments of the other legacy image, sprite and oracle tests replaced by measured ones that keep the 10 s floor. The head run then measured 65.47 s and 13.81 s.
- **MODE-02 coverage, D-22.** The success criterion asks for a fixture per mask combination. D-22 meets it in two parts: the eleven (R3, R4) pairs ($FF/$03, $7F/$07, $FF/$00, $FF/$01, $FF/$02, $80/$03, $9F/$03, $BF/$03, $DF/$03, $FE/$03 and $E0/$03) run in process under the closed-form oracle as `vdp.legacy.masks`, and two of those combinations are also assembled into ROM images, `legacy_modes_g2` (no mask bit limits the tables) and `legacy_modes_g2_masked` (R3 $9F, R4 $00, where a model that masks differs), run through `cogmaster-run` against the oracle's digest on three profiles.

## Task Commits

1. **Task 1: Tracer, legacy facts from source to the hardware note and the provenance rows, through the docs checks** - `f9cd1b1` (docs)
2. **Task 2: Embedding, conformance and the prep note** - `a6faf74` (docs)
3. **Task 3: Timeouts from the hosted run, phase-head evidence and the last push** - `6e8984e` (test); the push and the hosted run are this task's evidence, not files

**Plan metadata:** the SUMMARY commit and the state and roadmap commit that follow this file, both local; the push was the third scheduled one and is not repeated for them.

The tracer gate was met by re-running the Task 1 verify chain end to end after the last edit (both pins, the budgets, 71 docs, policy and single-version tests, exit 0) and then expanding.

## Files Created/Modified

- `docs/hardware/video.md` - the legacy paragraphs (modes, model, sprites, sources)
- `PROVENANCE.md` - sections 2, 4 and 10 rows for the two permissive file sets
- `docs/embedding.md` - kind 6 row and the revision 19 sentence
- `docs/conformance.md` - the legacy fixtures subsection
- `.planning/preparation/SMS-VDP.md` - section 7, the open question, the contents row, VDP.21, VDP.26, VDP.27, VDP.28 and the updated date
- `tests/runner/tests.cmake` - the `runner.hashes_file` comment and TIMEOUT
- `tests/fixtures/tests.cmake` - `legacy_timeout` for the late image and four measured comments

## Decisions Made

- **The TI page image was not checked** (assumption A2), so no OCR range is quoted: the docs say the manual's range fits above $E0 and $E0 or more alike.
- **Capture timestamp.** The research recorded no timestamp for the Young notes, so it came from the redirect of `curl -sI` on the `web/2026/` form (`20260307023015`); that capture answers 200.
- **g2_late shares one value.** The three tests take 29 s, twice the slowest of the group, as the existing comments group by their slowest member; the static and switch images keep the floor (0.51 s at most).
- **legacy_sprite_bounds left alone.** Plan 08 listed it among the timeouts to look at; it took 9.06 s on be32 against its 60 s tier (shared with `state.record_sweep` and `state.mapper`), which already exceeds twice, and `tests/core/tests.cmake` is not a file this plan lists. The value and its comment stay.

## Deviations from Plan

### Plan variants

- **One extra comment.** The provisional comment of `fixtures.legacy.oracle` in `tests/fixtures/tests.cmake` was replaced with its measured time (2 x 0.12 s) beside the sprite ones; the plan names the g2_late and sprite tests. No value changed.
- **Per-variant timeout.** The plan says to set the g2_late timeouts; the registration loop is shared by five variants, so a `legacy_timeout` variable was added instead of a second loop.
- **Contents row.** The "(later seed)" wording of the SMS-VDP contents row for section 7 and the file's `updated` date were changed with the section.

**Total deviations:** none under rules 1 to 4. No hosted job was red, so no triage round, no fourth push and no cause to record.
**Impact on plan:** none widens scope; `tests/perf/allow.txt`, `.github/workflows/ci.yml` and `cmake/packaging.cmake` are unchanged against `origin/main`.

## Issues Encountered

- The first `curl -sI` of the Young capture returned a 302 whose location carried the timestamp; the capture itself answered 200.
- The wait for the hosted run was done with `gh run watch` in the background and a bounded read of its output, since a foreground wait is moved to the background after 180 seconds.
- Flagged assumption kept open: the edge-probe row MODE-08 `concurrency` ("if interrupted or run in parallel, what is guaranteed?") does not apply to a documentation and provenance requirement and stays unresolved, as the plan lists.

## Evidence

Commands from the repository root; exit status 0 unless stated.

- `gh api repos/mamedev/mame/commits/1d7a454ed79e --jq .sha` and `gh api repos/ares-emulator/ares/commits/9408cb43d494 --jq .sha` (each answers 200, exit 0, run before the Task 1 commit and again in its verify chain); `curl -sI` of the Young capture URL, HTTP 200
- Task 1 verify chain: pins, `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` (887 check lines, 9738 src lines), `ctest --preset ci -R '^(docs\.|policy\.|release\.single_version_source)' -j8` (71 of 71)
- Task 2 verify chain: budgets, `ctest --preset ci -R '^(docs\.|policy\.)' -j8` (68 of 68), `wc -c` of embedding.md (37,204) and conformance.md (38,034) below 38,912
- phase head: `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake`; `git ls-files -z '*.c' '*.h' ':!:libretro/libretro.h' > build/p16-format.lst` then `xargs -0 /opt/homebrew/opt/llvm@21/bin/clang-format --dry-run --Werror < build/p16-format.lst`; `sh scripts/hygiene.sh --tree`; `cmake --workflow --preset ci` (737 of 737 in the test step); `cmake --workflow --preset hygiene` (84 of 84); `test -z "$(LC_ALL=C comm -23 <(sort build/p16-base-hashes.txt) <(sort build/ci/hashes.txt))"` (173 base lines, 198 head lines)
- `git diff --name-only origin/main -- .github/workflows/ci.yml cmake/packaging.cmake tests/perf/allow.txt` prints nothing; `git diff --quiet origin/main -- tests/perf/allow.txt` exit 0
- `git push origin phase/16-graphics-i-and-ii-draw` exit 0 (fast forward fba1e24 to 6e8984e, no force); `gh run list --branch phase/16-graphics-i-and-ii-draw --limit 1 --json headSha` equals `git rev-parse HEAD`
- hosted run on the head: https://github.com/szTheory/cogmaster/actions/runs/43564813364 (CI, success; 20 jobs, 18 success, rehearsal and cleanup skipped); `gh run view 43564813364 --json jobs --jq '.jobs[] | select(.name == "CI required") | .conclusion'` prints `success`; `perf` job success; the Nightly run 43564813421 on the same head also succeeded
- hosted perf, from the run log: `perf boot_to_play head=10089912 base=12288947 delta_bp=-1789 verdict=PASS`, no allow line
- hosted `sh scripts/ci-durations.sh tests 43564813364`: `runner.hashes_file` 65.47 s, `fixtures.legacy_modes.g2_late.sms2` 13.81 s (sms1 7.94, gg_sms 6.68), `fixtures.legacy_sprites.sms2` 1.31 s; the values it was sized from, run 43562888383: 64.72 s and 14.02 s

## Known Stubs

None. This plan edits documentation, provenance and test timeouts; nothing returns a placeholder.

## Threat Flags

None. No file under `src/`, `include/` or `runner/` changed and no endpoint, file access or trust boundary was added.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

The phase is complete and ready for `/gsd-ship`:

- Nine of nine plans have summaries; MODE-02, MODE-04, MODE-05, MODE-07 and MODE-08 close here, with MODE-01, MODE-03, MODE-06 and the three riders from plans 01 to 08.
- The branch `phase/16-graphics-i-and-ii-draw` is pushed through `6e8984e`; the pull request (number 47) is still a draft. The SUMMARY and state commits that follow are local and docs only, so the ship step pushes them. Do not mark the pull request ready and do not merge here.
- The pull request title is `feat(vdp): draw Graphics I and II on the Master System video profiles`; after `/gsd-ship`, `gh pr merge --auto --squash` runs in a shell and a CI run is confirmed on the head commit (rule 10).
- Open from earlier plans, in `deferred-items.md`: the `hygiene.tree` timing under load and the `release.firmware_scan` size rule that read a static library as a copier-headered image; neither recurred on this head (the hygiene workflow passed 84 of 84 and the ci workflow 737 of 737).

---
*Phase: 16-graphics-i-and-ii-draw*
*Completed: 2026-11-14*

## Self-Check: PASSED

The seven changed files exist; commits f9cd1b1, a6faf74 and 6e8984e are ancestors of HEAD and `gsd_run check evaluation-scope --plan 16-09 --commits-only` lists the same three; the acceptance counts were re-run on the final tree (video.md old sentence 0, tms9928a 3 and tms9918 3 in PROVENANCE, model 14 lines, tms9918a 2, Behaviour revision 19 once, the old kind 6 wording 0, legacy_sprites 4 lines in the conformance guide, the old VDP.21 note 0, the old R0 bit 3 line 0, the Young row and its licence regex once each, the run id in the hashes_file comment); the plan-level verification (ci and hygiene workflows, the subset compare, the budgets) exits 0; the hosted run on the pushed head exists and concluded success with CI required success; the coverage block uses block style with no flow mappings and no inline comments.
