---
phase: 10-bios-boot
plan: "11"
subsystem: close
tags:
  - bios
  - requirements
  - seeds
  - hosted-dispatch
status: complete

requires:
  - 10-02
  - 10-03
  - 10-05
  - 10-07
  - 10-09
  - 10-10
provides:
  - BIOS-01, BIOS-02 and BIOS-04 and ROADMAP criteria 1, 2 and 4 carry the wording of what was built; BIOS-01 to BIOS-05 are complete with their traceability rows
  - SEED-011 with every item the phase left out and its trigger, and the SEED-010 BIOS local judge as a predicates-only hand-off
  - quality.json floors tests.ci 626 and tests.wasm32 584
  - every timeout of the phase settled from hosted times (run 43074158512), none provisional
  - hosted dispatch D4 of ci.yml, a fresh main dispatch for perf and a nightly dispatch, all green on the final functional head
affects: []

actuals:
  tokens: 1900
  tasks: 3
  commits: 3

plan_head_before: 5d8ef0fbf399f593c083a710b56fcafedd6e23fe
plan_head_after: fb406766b5d3a92d80219f31f324e78bed1fe9f4
commits: 3

tech-stack:
  added: []
  patterns:
    - "Timeouts settled as max(20, ceil(2 x S)) from the slowest hosted time of the previous dispatch, each comment naming the lane, the number of runs and the run"

key-files:
  created:
    - .planning/seeds/SEED-011-bios-follow-ups.md
  modified:
    - .planning/seeds/SEED-010-pad-device-notes.md
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - .planning/STATE.md
    - quality.json
    - tests/host/tests.cmake
    - tests/runner/tests.cmake
    - tests/fixtures/tests.cmake
    - tests/bios/tests.cmake

key-decisions:
  - "The wording of BIOS-01, BIOS-02 and BIOS-04 and of roadmap criteria 1, 2 and 4 is amended to what was built, as the earlier phases amended theirs, in the closing plan"
  - "Every remaining provisional timeout of the phase takes the 20 s floor: the slowest hosted time of any of them was 1.42 s (be32), so 2 x S is far below the floor and no trim was needed"
  - "The local real-BIOS and window runs are not run by the agent: they read the owner's dumps, which the session rule bars, so they stay an owner hand-off recorded in SEED-010"

requirements-completed: []

coverage:
  - id: C1
    description: "The two firmware legality scans (release and local window) land before any BIOS code, each with a negative control; the release scan and its controls run in ci, the window scan runs in the library preset and the pre-commit and pre-push hooks and its controls run in the hygiene lane; the wording of the requirement and of roadmap criterion 1 says exactly that"
    requirement: BIOS-01
    verification:
      - kind: integration
        ref: "grep -c for the amended text in REQUIREMENTS.md and ROADMAP.md prints 1 each and the superseded wording is gone (exit 0); cmake --workflow --preset ci exit 0 and cmake --workflow --preset hygiene exit 0 on the final head; hosted run 43076225840 success; the commit order was evidenced by merge-base exit statuses in plan 03"
        status: pass
    human_judgment: false
  - id: C2
    description: "A user boots a Master System from their own 8 KiB dump with --boot bios --bios FILE; fast boot stays the default with every existing hash unchanged, the dump is named by SHA-1, and the runner exits 77 for a required and absent or unsupported BIOS; the wording says so and names the Game Gear and built-in-game BIOSes as seeds"
    requirement: BIOS-02
    verification:
      - kind: integration
        ref: "ctest runner.bios.* and host.bios in the ci workflow exit 0; git diff --quiet against the merge base over tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake tests/fuzz/fuzz_run.c tests/fuzz/fuzz_loader.c exit 0; hosted run 43076225840 success"
        status: pass
    human_judgment: false
  - id: C3
    description: "CI runs the self-authored BIOS-shaped image with no skip, a classification-complete equivalence of the fast-boot hand-off and the state that image reaches, hostile images and the BIOS fuzz harness; the nightly libFuzzer run on the BIOS harness passes"
    requirement: BIOS-03
    verification:
      - kind: integration
        ref: "ctest bios.equivalence and its two controls, bios.hostile and fuzz.regress.bios in the ci workflow exit 0; nightly run 43076276567 success with fuzz.libfuzzer.bios Passed in 61 s"
        status: pass
    human_judgment: false
  - id: C4
    description: "The libretro information file declares the firmware as optional (three Master System file names, the Game Gear BIOS a seed) and the in-process host test shows the adapter reads it from the system folder when the boot option asks for it; the one local RetroArch run handed the adapter its system folder and honoured the options file"
    requirement: BIOS-04
    verification:
      - kind: integration
        ref: "ctest libretro.host.bios and libretro.info_claims in the ci workflow exit 0; the repository's own RetroArch harness with a temporary system folder and a core-options file exit 0 for 2 and 3 frames (see Local judgments)"
        status: pass
    human_judgment: false
  - id: C5
    description: "SMS-SYSTEM section 3 is checked against the code and each BIOS and cartridge enable combination under 3E has a test with the documented result; docs/bios.md and the README sentence were verified here against the code and not edited"
    requirement: BIOS-05
    verification:
      - kind: integration
        ref: "ctest bios.arbitration and the docs and policy tests exit 0 in the ci workflow; ctest -R docs.api_names exit 0; the planning-identifier grep over docs and README additions prints 0"
        status: pass
    human_judgment: false
  - id: C6
    description: "The floors tests.ci and tests.wasm32 equal the registered counts minus 2 (626 and 584) and policy.quality passes; every timeout the phase added is settled and the count of provisional comment lines in each touched registration file is at most its count at origin/main"
    requirement: BIOS-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^policy\\.quality' exit 0 (10 tests); the provisional-count loop over nine registration files exit 0; the settled-comment pattern count rises in tests/host, tests/runner, tests/fixtures, tests/bios and tests/libretro"
        status: pass
    human_judgment: false
  - id: C7
    description: "A real Sega BIOS leaves residue fast boot lacks (logo tiles, VDP registers, work RAM above C000, Z80 registers and R, IM and IFF, PSG attenuation, beam phase, paging after a large checksum); this is verified only on the owner's own dump in the local library preset and recorded as predicates"
    requirement: BIOS-03
    verification:
      - kind: manual_procedural
        ref: "the local judge in SEED-010 (hand-off); the window scan and the real-dump runner commands were not run here: not run, no BIOS directory is read by the agent"
        status: not_run
    human_judgment: true

duration: about 60 minutes
completed: 2026-11-11
---

# Phase 10 Plan 11: BIOS boot close Summary

The requirement and roadmap wording now says what was built, BIOS-01 to BIOS-05 are complete, the floors are 626 and 584, SEED-011 records every deferred BIOS item with its trigger, every timeout of the phase is measured, and the final head is green on a hosted dispatch, a fresh main dispatch (perf delta -11 basis points) and the nightly with libFuzzer on the BIOS harness.

## Accomplishments

- `.planning/seeds/SEED-011-bios-follow-ups.md` (new): the Game Gear 1 KiB BIOS with its stated way forward (a native-mode `$3E` latch only when a BIOS is attached, the bit 6 fix behind its own test with `git diff --quiet origin/main -- tests/io` and the ledger as the movement check, the 1,024-byte window, `bios.gg` with the code that reads it), built-in-game BIOSes, an empty-slot or test-card BIOS boot, a region hint, a Mark III profile, a hosted RetroArch BIOS session (YAML only), CI `hygiene.sh --history` (25 commits scan in 4.0 s on macOS; the hygiene job timeout must be re-measured and `fetch-depth: 0` added) and a publish-job post-download scan. No path, name, digest or byte of a dump.
- `.planning/seeds/SEED-010-pad-device-notes.md`: gained "The BIOS local judge (hand-off, not a requirement)"; no line removed.
- `.planning/REQUIREMENTS.md` and `.planning/ROADMAP.md`: BIOS-01, BIOS-02, BIOS-04 and criteria 1, 2 and 4 reworded; BIOS-01 to BIOS-05 `[x]` and `Complete`; Phase 10 plan list and the top-of-file line ticked (completed 2026-11-11).
- `quality.json`: floors `tests.ci` 626 and `tests.wasm32` 584 (non-comment inventory lines 628 and 586, minus 2); no reason entry needed for a rise.
- The last timeouts, below.

## Task Commits

1. Task 1, the seeds: `3cc9c58`
2. Task 2, the amended wording, the floors and the STATE row: `5df9830`
3. Task 3, the settled timeouts: `fb40676`

## Timeouts

Read from the D3 run 43074158512 (https://github.com/szTheory/cogmaster/actions/runs/43074158512) with a script over `gh run view 43074158512 --log`; S is the slowest passed time, N the number of lanes read. Every test is settled as `max(20, ceil(2 x S))`, so each takes the 20 s floor; no trim was needed on the be32 lane.

| Test | File | S | Lane | N | Timeout |
|---|---|---|---|---|---|
| host.bios | tests/host/tests.cmake | 0.07 s | wasm32 | 9 | 20 s floor |
| runner.bios.grammar, .refusals, .no_path_echo | tests/runner/tests.cmake | 0.61 s | be32 | 9 | 20 s floor |
| runner.bios.boot | tests/fixtures/tests.cmake | 0.27 s | be32 | 9 | 20 s floor |
| bios.equivalence | tests/fixtures/tests.cmake | 1.42 s | be32 | 9 | 20 s floor |
| bios.equivalence.bad_ram, .bad_sp | tests/fixtures/tests.cmake | 0.21 s | be32 | 9 | 20 s floor |
| fixtures.bios_shape.pin.* (four) | tests/fixtures/tests.cmake | 0.06 s | windows-arm64 | 9 | 20 s floor |
| bios.hostile | tests/bios/tests.cmake | 0.43 s | be32 | 9 | 20 s floor |

The other `bios.*` tests (arbitration, slow_flag, load, state) were already settled in plan 05 and the two libretro tests in plan 10. The provisional-count loop over `tests/host`, `tests/runner`, `tests/fixtures`, `tests/libretro`, `tests/bios`, `tests/policy/firmware_release_cases.cmake`, `tests/policy/firmware_window_cases.cmake`, `tests/hygiene` and `tests/fuzz` exits 0: counts now 0, 15, 13, 0, 0, 0, 0, 2 and 2, against 0, 15, 13, 0, 0, 0, 0, 2 and 2 at origin/main. The settled-comment pattern count rose in all five of host (3 against 2), runner (17 against 16), fixtures (19 against 15), bios (5 against 0) and libretro (4 against 2).

Tests whose hosted time exceeds half their timeout: none among the tests the phase added or changed (policy.bios_*, policy.firmware_*, release.firmware_scan, hygiene.byte_run.*, bios.*, abi.api_v14, fixtures.bios_shape.*, fuzz.regress.bios, host.bios, runner.bios.*, libretro.host.bios): the slowest ratio is libretro.host.bios at 1.7 s of 20 s on be32 in the final run. Tests of earlier phases that exceed half their registered timeout on be32 (for example libretro.host.fm_option, mapper.ext.hes_4pak, runner.hashes_file) are not this phase's and were not edited. No job exceeds half its `timeout-minutes` (the slowest, build windows-arm64, took 176 s of a 360 s limit, and be32 183 s of 600 s), so `.github/workflows/ci.yml` is not edited. `fuzz.libfuzzer.bios` took 61.13 s in the nightly; the libFuzzer timeouts sit in `tests/fuzz/tests.cmake` under the existing provisional comments of the earlier phases and were left alone.

## Local judgments

- One local RetroArch observation (RetroArch.app installed). Commands from the repository root, each the repository's own harness `cmake -DRETROARCH=/Applications/RetroArch.app/Contents/MacOS/RetroArch -DCORE=build/ci/libretro/cogmaster_libretro.dylib -DINFO=build/ci/libretro/cogmaster_libretro.info -DTEMPLATE=tests/retroarch/test.cfg.in -DRA_DIR=<dir> -DKEEP=1 -DFRAMES=N -DSHOT=<png> -DCONTENT=build/ci/fixtures/bios_probe.sms -P tests/retroarch/run_retroarch.cmake`, with the self-authored `bios_shape.bin` copied into a temporary system folder under `build/` as `bios_U.sms` and a core-options file holding `cogmaster_boot = "bios"`: exit 0 at 2 frames and at 3 frames. The RetroArch log shows `GET_SYSTEM_DIRECTORY` answered with the configured system folder, and holds no warning line. The frame at 2 frames differs from the fast-boot frame at 2 frames (`cmp -s` exit 1) and the frame at 3 frames equals the fast-boot frame (the BIOS phase is over), so RetroArch handed the adapter its system folder and honoured the options file. It decides nothing in the CI path; no fix or seed followed from it.
- `sh scripts/firmware-window.sh` and the runner with a real dump (`cogmaster-run <cartridge> --boot bios --bios <dump> --frames 600`): not run. They read the owner's own dumps, which this session may not touch; this is the only thing that needs the owner's own machine, and it stays the hand-off recorded in SEED-010 (predicates only: the recognised label or SHA-1 fact, exit 0 or 77, and `K compared`). Nothing local was committed.

## Unchanged proof

- `git diff --quiet $(git merge-base origin/main HEAD) -- tests/io tests/vdp tests/bus tests/audio tests/ports tests/testcard_pins.cmake tests/fuzz/fuzz_run.c tests/fuzz/fuzz_loader.c` exit 0 (merge base `202f107bf1e888d197ed090f34b01cce73e7185c`, equal to origin/main).
- `! git diff $(git merge-base origin/main HEAD) -- tests/built/expected.txt tests/diag/expected.csv tests/roms/manifest.txt | grep -q '^-[^-]'` exit 0 (no line removed).
- `git diff --quiet origin/main -- tests/io tests/audio tests/testcard_pins.cmake` exit 0; `git diff --quiet origin/main -- tests/perf/allow.txt` exit 0.

## Budgets

`cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` exit 0: 833 counted check lines (cap 847), 9,399 src lines (strict one tenth). `wc -c include/cogmaster.h` 9,951 (under 24,576). Docs margins to 38,912: `docs/testing.md` 36,714 (2,198 under), `docs/embedding.md` 36,379 (2,533 under), `docs/runner.md` 32,710, `docs/conformance.md` 29,837; both testing and embedding are at most 36,864. `docs/bios.md` 7,132 bytes and `README.md` 4,095. `cat tests/retroarch/* tests/fixtures/play.h | wc -l` prints 275 (cap 300). `tests/fuzz` holds 73,526 bytes (cap 524,288). `ctest -R '^docs\.api_names$'` passes; `grep -r COGMASTER_BIOS_DIR tests` finds 0 matches; the planning-identifier grep over the docs and README additions prints 0. `cmake --workflow --preset ci` exit 0 (623 tests and the 5 of the second stage) and `cmake --workflow --preset hygiene` exit 0, both on the head of the timeout commit; `cmake --workflow --preset asan` exits 1 locally at configure (no clang-21 here), so the hosted asan job decides: success in the run below.

## Final hosted run

All three dispatched on pushed heads equal outside `.planning` to the final functional head `fb406766b5d3a92d80219f31f324e78bed1fe9f4` (`git diff --quiet fb406766b5d3a92d80219f31f324e78bed1fe9f4 HEAD -- . ':(exclude).planning'` exit 0; the only changes after it are this SUMMARY and the planning records). One attempt each; no re-dispatch and no first-step DNS failure.

- Branch ci dispatch (D4): https://github.com/szTheory/cogmaster/actions/runs/43076225840, `gh run watch --exit-status` exit 0, conclusion success. Jobs success: changes 5 s, hygiene 61 s, be32 183 s, perf 74 s, nofp (ubuntu-26.04) 11 s, nofp (ubuntu-26.04-arm) 20 s, asan 90 s, retroarch 46 s, wasm32 67 s, hash-equality 7 s, `build (linux-arm64)` 51 s, `build (linux-x64)` 59 s, `build (macos-arm64)` 80 s, `build (macos-x64)` 155 s, `build (windows-x64)` 151 s, `build (windows-arm64)` 176 s, CI required 3 s; skipped as allowed: title, rehearsal, cleanup. The `jq -e` filter over `hash-equality|asan|nofp|retroarch|wasm32|be32|perf` exit 0 (8 jobs) and over `^build` exit 0 (6 jobs).
- Fresh main dispatch: https://github.com/szTheory/cogmaster/actions/runs/43076272120, head SHA `202f107bf1e888d197ed090f34b01cce73e7185c` equal to `git rev-parse origin/main`, conclusion success (an earlier main run of the same head, 43063808940, read B = 12303282 as well).
- Perf (a dispatch prints the control line only): branch `perf control boot_to_play runs=12288943,12288943,12288943 spread=0`; main `perf control boot_to_play runs=12303282,12303282,12303282 spread=0`.

perf dispatch boot_to_play H=12288943 B=12303282 delta_bp=-11

- Nightly dispatch: https://github.com/szTheory/cogmaster/actions/runs/43076276567, head `fb406766b5d3a92d80219f31f324e78bed1fe9f4` (equal to the branch dispatch), conclusion success. Jobs success: fuzz (libFuzzer on loader, run, save, eeprom, pad and bios; `fuzz.libfuzzer.bios` Passed in 61.13 s and `fuzz.libfuzzer.run` in 61.15 s, the first hosted libFuzzer run of the BIOS harness), bench (ubuntu-26.04), bench (ubuntu-26.04-arm), bench (macos-15), vectors, bench-wasm32, report.

## Ship-time confirmations

`/gsd-ship` (or the session after the merge) runs these from the repository root and records each command, its exit status and the run URL. Items 0 to 3 need the phase pull request; item 4 needs the merge.

0. Hash equality on the pull request head. `gh pr view phase/10-bios-boot --json headRefOid --jq .headRefOid > "$TMPDIR/head.txt"`; `gh run list --workflow ci.yml --commit "$(cat "$TMPDIR/head.txt")" --event pull_request --json databaseId,url > "$TMPDIR/head-runs.json"` and `jq -e 'length > 0' "$TMPDIR/head-runs.json"`; `gh run watch <id> --exit-status`; `gh run view <id> --json jobs > "$TMPDIR/head-jobs.json"`; `jq -e '[.jobs[] | select(.name | test("hash-equality|retroarch"))] | length >= 2 and all(.conclusion == "success")' "$TMPDIR/head-jobs.json"` (exit 0). Append the unchanged-proof commands with their exit statuses and the hash-equality run URL to the pull request body (`gh pr view ... --json body --jq .body > "$TMPDIR/body.md"`, edit, `gh pr edit ... --body-file "$TMPDIR/body.md"`). Also confirm the release scan ran on the head: `gh run view <id> --log > "$TMPDIR/head.log"` and `grep -E 'release\.firmware_scan .*Passed' "$TMPDIR/head.log" | wc -l` is at least 6.
1. Perf gate on the pull request. `gh run view <id> --log > "$TMPDIR/pr.log"` and `grep -Eq 'perf boot_to_play head=[0-9]+ base=[0-9]+ delta_bp=-?[0-9]+ verdict=PASS' "$TMPDIR/pr.log"` (exit 0); record the line in the pull request body.
2. AGENTS rule 10: title, auto-merge, CI on the head. `gh pr view phase/10-bios-boot --json title --jq .title > "$TMPDIR/title.txt"` and `grep -Eq '^(feat|fix|docs|ci|test|refactor|perf|build|chore)(\([a-z0-9-]+\))?!?: ' "$TMPDIR/title.txt"`; if it exits 1, `gh pr edit phase/10-bios-boot --title "feat: boot from a BIOS dump"` and re-check. In a shell, never inside GSD command text: `gh pr merge phase/10-bios-boot --auto --squash` (exit 0); then confirm a CI run exists on the head commit (item 0).
3. Fuzz lane on the head. The `fuzz` job lives in `nightly.yml`: `gh run list --workflow nightly.yml --commit "$(cat "$TMPDIR/head.txt")" --event pull_request --json databaseId > "$TMPDIR/nightly-runs.json"`, `jq -e 'length > 0' "$TMPDIR/nightly-runs.json"` (exit 0), `gh run watch <id> --exit-status`, `gh run view <id> --json jobs > "$TMPDIR/nightly-jobs2.json"` and `jq -e '[.jobs[] | select(.name == "fuzz")] | length >= 1 and all(.conclusion == "success")' "$TMPDIR/nightly-jobs2.json"` (exit 0). If the pull request has no nightly run, use the dispatched nightly run above when its head equals the pull request head outside `.planning`. The job is `continue-on-error`: a failure is triage, recorded with its URL.
4. Release pull request green after the merge. `gh pr list --label 'autorelease: pending' --json number,url,statusCheckRollup > "$TMPDIR/release-pr.json"` and `jq -e 'length == 1 and (.[0].statusCheckRollup | all((.conclusion // .state) | IN("SUCCESS", "SKIPPED", "NEUTRAL")))' "$TMPDIR/release-pr.json"` (exit 0); afterwards `gh run list --workflow release.yml --branch main --limit 1 --json conclusion,url` shows success.
5. Allow line. No `tests/perf/allow.txt` line was added in this phase (`git diff --quiet origin/main -- tests/perf/allow.txt` exit 0).

## Deviations from Plan

### Interpretations (no behaviour lost)

- **Floors from the whole inventory, not the plan's estimate.** The plan's text expects about 623 and 582; the non-comment line counts of the inventories are 628 and 586, so the floors are 626 and 584 by the stated rule (count minus 2). The ci workflow runs 623 tests in the first stage, the inventory counts the others its lanes add.
- **ROADMAP plan count line.** The planning step had left `**Plans**: 10/11 plans executed`, not `0/11 plans complete`; it was replaced by `**Plans**: 11/11 plans complete`, which is one more such line than at origin/main as the plan expects.
- **`requirements-completed` is empty.** As the plan specifies, each requirement was credited by the plan that completed it (BIOS-01 plan 03, BIOS-05 plan 05, BIOS-03 plan 07, BIOS-02 plan 09, BIOS-04 plan 10); this plan marks all five complete in REQUIREMENTS.md by hand edit (scoped `Edit`, as the earlier closing plans did) and proves them together. `gsd-tools query requirements.mark-complete BIOS-01 BIOS-02 BIOS-03 BIOS-04 BIOS-05` then reported all five as already complete (no block on BIOS-03 being declared by two plans) and changed nothing.
- **Local real-BIOS runs not made.** The plan's local judgment (b) runs only with the owner's BIOS directory; this session may not read, hash or run against a real dump, so they were not run and stay the owner hand-off in SEED-010.
- **Fresh main dispatch although a same-head run existed.** Run 43063808940 had the same head and the same control value; a fresh dispatch (43076272120) was made anyway, as the closing instructions asked, and gave the identical value.

**Total deviations:** 0 rule-driven fixes; 5 interpretations recorded above. **Impact:** none on the requirements.

## Known Stubs

None.

## Threat Flags

None. This plan adds no code, only planning records, floors and timeout values.

## Phase notes

- The amendments land here, in the closing plan, as the Phase 7, 8 and 9 wording amendments did.
- The user-facing prose landed with the code it describes (the guide and the README in plan 10, the runner guide in plan 09, the embedding text in plans 04 and 05, the testing-guide rows in plan 06, the provenance and environment text in plan 08); `docs/bios.md` was therefore verified here against the code and not edited.
- No extra dispatch was made after plan 07: the tests of plans 06 and 07 were first seen hosted at D3 and their timeouts are settled here from D3's times (the libretro ones in plan 10), within ten files per plan.
- The equivalence proves the mechanism and the single source of constants, not what Sega's BIOS leaves; the real-BIOS residue is verified only on the owner's own dump and recorded as predicates (SEED-010).

## Self-Check: PASSED

- This SUMMARY exists; `.planning/seeds/SEED-011-bios-follow-ups.md` exists.
- Commits `3cc9c58`, `5df9830` and `fb40676` are ancestors of HEAD.
