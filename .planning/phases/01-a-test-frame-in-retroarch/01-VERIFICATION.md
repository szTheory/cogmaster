---
phase: 01-a-test-frame-in-retroarch
verified: 2026-10-19T23:47:27Z
status: passed
score: 15/15 must-haves verified
covered_files:
  - ".github/workflows/ci.yml"
  - ".github/workflows/nightly.yml"
  - ".github/workflows/release.yml"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-01-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-01-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-02-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-02-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-03-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-03-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-04-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-04-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-05-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-05-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-06-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-06-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-07-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-07-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-08-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-08-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-09-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-09-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-10-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-10-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-11-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-11-SUMMARY.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-12-PLAN.md"
  - ".planning/phases/01-a-test-frame-in-retroarch/01-12-SUMMARY.md"
  - ".release-please-manifest.json"
  - "CMakeLists.txt"
  - "CMakePresets.json"
  - "PROVENANCE.md"
  - "README.md"
  - "THIRD-PARTY-NOTICES.md"
  - "cmake/CogmasterTests.cmake"
  - "cmake/cogmaster.pc.in"
  - "cmake/cogmasterConfig.cmake.in"
  - "cmake/packaging.cmake"
  - "cmake/toolchains/ppc-linux-gnu.cmake"
  - "cmake/toolchains/wasm32-wasi.cmake"
  - "cmake/version.h.in"
  - "docs/embedding.md"
  - "docs/maintaining.md"
  - "docs/runner.md"
  - "docs/testing.md"
  - "host/palette.c"
  - "host/palette.h"
  - "include/cogmaster.h"
  - "libretro/CMakeLists.txt"
  - "libretro/cogmaster_libretro.info.in"
  - "libretro/libretro.c"
  - "libretro/libretro.h"
  - "pins.txt"
  - "release-please-config.json"
  - "runner/bench.c"
  - "runner/bench.h"
  - "runner/clock.c"
  - "runner/clock.h"
  - "runner/crc32.c"
  - "runner/crc32.h"
  - "runner/exit_status.h"
  - "runner/main.c"
  - "runner/png.c"
  - "runner/png.h"
  - "runner/rss.c"
  - "runner/rss.h"
  - "runner/sha256.c"
  - "runner/sha256.h"
  - "runner/stop.c"
  - "runner/stop.h"
  - "scripts/apply-repo-settings.sh"
  - "scripts/check-title.sh"
  - "scripts/classify-changes.sh"
  - "scripts/publish.sh"
  - "scripts/setup-release-app.sh"
  - "src/crc32.c"
  - "src/fault.c"
  - "src/frame.c"
  - "src/instance.c"
  - "src/internal.h"
  - "src/state.c"
  - "src/state.h"
  - "src/testcard.c"
  - "tests/abi/global_fixture.c"
  - "tests/abi/libc_fixture.c"
  - "tests/abi/nofp_fixture.c"
  - "tests/abi/tests.cmake"
  - "tests/api/v1.c"
  - "tests/check.h"
  - "tests/cmake/abi.cmake"
  - "tests/cmake/action_pins.cmake"
  - "tests/cmake/api_names.cmake"
  - "tests/cmake/api_version.cmake"
  - "tests/cmake/archive_contents.cmake"
  - "tests/cmake/budgets.cmake"
  - "tests/cmake/check_hash.cmake"
  - "tests/cmake/check_junit.cmake"
  - "tests/cmake/ci_required.cmake"
  - "tests/cmake/e2e_budget.cmake"
  - "tests/cmake/exit_table.cmake"
  - "tests/cmake/expect_fail.cmake"
  - "tests/cmake/floor.cmake"
  - "tests/cmake/hashes.cmake"
  - "tests/cmake/info_claims.cmake"
  - "tests/cmake/install_line.cmake"
  - "tests/cmake/no_float.cmake"
  - "tests/cmake/no_skip.cmake"
  - "tests/cmake/notices.cmake"
  - "tests/cmake/perf_compare.cmake"
  - "tests/cmake/pins.cmake"
  - "tests/cmake/single_version.cmake"
  - "tests/cmake/timeouts.cmake"
  - "tests/cmake/vendored.cmake"
  - "tests/consumer/CMakeLists.txt"
  - "tests/consumer/main.c"
  - "tests/consumer/main.cpp"
  - "tests/core/test_core.c"
  - "tests/core/test_state.c"
  - "tests/core/tests.cmake"
  - "tests/embed/CMakeLists.txt"
  - "tests/header/header.c"
  - "tests/header/header.cpp"
  - "tests/host/test_palette.c"
  - "tests/host/tests.cmake"
  - "tests/hygiene/fixtures/titles.txt"
  - "tests/hygiene/tests.cmake"
  - "tests/install/tests.cmake"
  - "tests/inventory/ci.txt"
  - "tests/inventory/hygiene.txt"
  - "tests/inventory/nofp.txt"
  - "tests/inventory/wasm32.txt"
  - "tests/libretro/host.c"
  - "tests/libretro/tests.cmake"
  - "tests/perf/allow.txt"
  - "tests/perf/tests.cmake"
  - "tests/policy/tests.cmake"
  - "tests/release/tests.cmake"
  - "tests/retroarch/bmp.c"
  - "tests/retroarch/compare.c"
  - "tests/retroarch/flip.cmake"
  - "tests/retroarch/run_retroarch.cmake"
  - "tests/retroarch/test.cfg.in"
  - "tests/runner/test_png.c"
  - "tests/runner/test_runner.c"
  - "tests/runner/tests.cmake"
  - "tests/testcard_pins.cmake"
  - "tests/tests.cmake"
  - "version.txt"
covered_digest: "v3:sha256:a23bf324fe91c5822d1910aff9e7087dc583f461843a76c5a0b4ed2d365875bf"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 1: A test frame in RetroArch Verification Report

**Phase goal:** A host can build, install and run cogmaster on every platform and get the same test-card frame from
the library, the runner and the libretro core, before any emulation exists; merging the pull request publishes the
release.
**Verified:** 2026-10-19T23:47:27Z
**Status:** passed
**Re-verification:** No, this is the first verification

Under project rules 6, 9 and 10, the real publish on push to `main` is confirmed at ship. It is not a human item.
Everything this phase can show before a merge was checked by commands run here, by hosted CI runs, or by reading the
code.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | SC1 / FRAME-01: `cmake --workflow --preset ci` (`ci-msvc` on Windows) exits 0 on all six platforms, and a test shows the exact `tests/inventory/ci.txt` inventory ran once each with no skipped test | ✓ VERIFIED | I ran it locally from a removed `build/ci`: exit 0, "100% tests passed out of 80" plus "out of 4" (release). The 84 executed names equal the 84 inventory names (diff of names, ignoring labels). The cpack step made three zips. Hosted run 39337625991 (head 79b6f19) has all six `build (...)` legs green. Each leg runs `check_junit.cmake` against `ci.txt` for the main and the release JUnit files (ci.yml:151,155). |
| 2 | SC2 / FRAME-02: `--frames 60 --hash` gives the same SMS hash, and `--machine gg` the same GG hash, on six platforms, on be32 under qemu and on wasm32 under wasmtime | ✓ VERIFIED | The local runner prints `e14d77ce…7ae7` (SMS) and `7404ed4a…e779` (GG), both equal to `tests/testcard_pins.cmake`. I wrote my own Python version of the card from the rules in `src/testcard.c` and it gives the same two hashes. Its GG chart holds 4,096 distinct colours. Hosted run 39337625991: hash-equality logged "hashes: 7 identical files" (six legs plus be32), and wasm32 logged "wasm32 hashes.txt equals Linux x64's". |
| 3 | SC3 / FRAME-03: an in-process libretro host runs 60 frames, its frame equals the runner's byte for byte, and av_info reports 1,640,625/27,379 Hz and the SMS or GG geometry | ✓ VERIFIED | `tests/libretro/host.c` checks `fps == 1640625.0/27379.0`, the base and max geometry and the aspect (lines 432-438). It hashes frame 60 with the runner's SHA-256 against the same pinned hash the runner tests use. `libretro.host.xrgb8888`, `.rgb565` and `.gg` all pass locally and are in `ci.txt`. `src/instance.c:161-162` sets fps 1640625/27379. |
| 4 | SC4 / FRAME-06: the PR prerelease rehearsal produces exactly 18 version-free archives, `SHA256SUMS` and verified attestations, then deletes itself; a packaging test finds LICENSE and THIRD-PARTY-NOTICES.md in each archive | ✓ VERIFIED | Run 39337625991 rehearsal log: "18 archives from 242d4e7…; SHA256SUMS written". All 18 uploaded. Each archive "verifies; a copy with its last byte flipped fails". The release was "published with 19 assets; every download matches SHA256SUMS and verifies". Cleanup: "rehearsal-39337625991-1 and its release are gone". `gh release list` and the tags API both return nothing now. `release.archive_contents` and its self-test pass. The local libretro zip holds LICENSE, THIRD-PARTY-NOTICES.md, `cores/` and `info/`, and the archive names carry no version. |
| 5 | SC5 / FRAME-07: the README install line, run in CI against the built macOS arm64 archive, checks the checksum and installs core and `.info` where RetroArch 1.22.2 finds them; the RetroArch screenshot equals the runner frame; the one-byte negative control fails | ✓ VERIFIED | Run 39337625991, macos-arm64 leg: "installed cores/cogmaster_libretro.dylib and info/cogmaster_libretro.info" under `~/Library/Application Support/RetroArch`, and "checksum refused the tampered zip (exit 1)". The README line is `shasum -c … && tar`. The retroarch job logged "RetroArch 1.22.2" (asserted by `test "$version" = 1.22.2`) and then "256x192, every pixel equal". In the flip control, "compare exited 1: first difference at 7,3". `release.yml` `install-line-latest` runs the line against `releases/latest/download/` after publish. |
| 6 | FRAME-04: nm-based ABI tests (only `cogmaster_*` exports, no mutable globals, only the four memory imports plus helpers, no floating point), the header compiles as C17 and C++17, header < 24 KB, README < 12 KB | ✓ VERIFIED | `abi.symbols`, `abi.globals`, `abi.undefined` (with self-tests), `abi.header.c17`, `abi.header.cxx17` and `docs.budgets` pass locally and on every leg. The `nofp` jobs are green on both Linux arches. `include/cogmaster.h` is 3,588 B and `README.md` 3,315 B. The import allowlist in `tests/cmake/abi.cmake` is `memcpy`, `memmove`, `memset` and `memcmp` plus named per-OS helpers. |
| 7 | FRAME-05: hygiene tree, title check, timeout-comment check, SHA-pin check and no-self-skip run in CI, each with a negative control | ✓ VERIFIED | `tests/inventory/hygiene.txt` lists `hygiene.tree`, `policy.title`, `hygiene.timeout_comments` and `policy.action_pins`, each with a `.self_test`. `policy.no_skip` and its `.self_test` are in `ci.txt`. The hygiene and title jobs are green in all three hosted runs. |
| 8 | FRAME-08: installed-package C consumer, C++ consumer and `add_subdirectory` embed build in CI on every platform | ✓ VERIFIED | `install.consumer`, `install.consumer.cxx`, `install.consumer.dll_crt` and `embed.subdirectory` are in `ci.txt`, pass locally, and are run on all six legs (check_junit). `tests/consumer/CMakeLists.txt` builds both a `find_package` consumer and a pkg-config consumer. |
| 9 | FRAME-09: `--bench` reports p50…p99.999 and max RSS; Cachegrind Ir/frame runs per PR against the merge base with the +1 % gate present and report-only; zero allocations after load; the core never reads a host clock | ✓ VERIFIED | `cogmaster-run --bench 600` prints `p50_us` through `p99.999_us`, `max_us` and `max_rss_kib`. The perf job in 39337625991 printed `perf testcard head=2146896 base=none … verdict=NO_BASE`; `main` has no code yet, so there is no base to compare. `perf.gate.plus_1_00` and `plus_1_01` pass. `core.no_alloc_after_load` passes. `abi.undefined` allows no clock import. |
| 10 | Goal clause: merging the PR publishes the release with no manual step (static half; the real publish is confirmed at ship) | ✓ VERIFIED | `release.yml` on push to `main` runs release-please with the App token, auto-merges the release PR, runs `ci.yml` via `workflow_call` at the release sha, then publishes and checks the install line. `release-please-config.json` matches the plan. `uvx check-jsonschema --builtin-schema vendor.github-workflows` reports "ok -- validation done" for all three workflows. SchemaStore's workflow schema lists `concurrency.queue` with `max` as an allowed value, which answers review WR-03's open question. actionlint 1.7.12 rejects `queue`, but that is because its schema is older. The App secrets are configured, since the rehearsal minted a token. `allow_auto_merge` is true. |
| 11 | Backstop (plan 03): a signal never leaves a half-written PNG | ✓ VERIFIED | Observed directly. `build/ci/tests/cogmaster-test-run-stop` raises SIGTERM inside frame 3. With `--png 3:… --png 60:…` it exited 0 and wrote a frame-3 PNG that is byte-identical to a normal `--frames 3 --png 3:` run (SHA-256 `c4d1160f…`, the pinned PNG hash). It wrote no frame-60 file. |
| 12 | Backstop (plan 04): a signal never leaves a truncated report | ✓ VERIFIED | `runner.stop` passes. It checks that the hooked runner prints `frame 3 sha256 <pin>` for `--hash`, and `frames 3 … max_rss_kib N` for `--bench`, both with exit 0. |
| 13 | Backstop (plan 07): `find_package` works with or without pkg-config, and `cogmaster::cogmaster` carries both header directories | ✓ VERIFIED | `install.consumer` builds the find_package consumer and the pkg-config consumer in one project against the staged install, and passes on all six legs. |
| 14 | Backstop (plan 09): each RetroArch run gets a fresh directory, its own HOME and a 50 s bound | ✓ VERIFIED | Observed directly. I drove `run_retroarch.cmake` twice with stand-in binaries. In run 1, the stand-in wrote a screenshot and a stray file. Run 2 failed with "RetroArch wrote no screenshot", so the stale shot had been removed, and the stray file was gone. The child saw `HOME=<RA_DIR>/home`. `TIMEOUT 50` is set at line 44. |
| 15 | Backstop (plan 10): each rehearsal uses a unique tag, cleanup always runs, and a leftover rehearsal is never latest | ✓ VERIFIED | The tag was `rehearsal-39337625991-1`. Cleanup has `if: always() && needs.rehearsal.result != 'skipped'`, and publish uses `-F prerelease=true -f make_latest=false`. No release or tag remains after four PR runs, one of which failed. |

**Score:** 15/15 truths verified, none present but behaviour-unverified.

The plan-level truths in all 12 plans map to named tests in `tests/inventory/ci.txt` and `hygiene.txt`, all of which
passed locally and in hosted CI. The plans' `verify.artifacts` checks pass 37/37 (exit 0 for every plan), and their
`verify.key-links` checks pass 23/23.

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/testcard.c` | integer card for SMS and GG | ✓ VERIFIED | Pure function of (x, y, machine). An independent recomputation matches the pins. |
| `include/cogmaster.h` | public API | ✓ VERIFIED | 9 functions and size-tagged structs. The ABI tests pass. |
| `runner/main.c` and friends | `--frames`, `--hash`, `--png`, `--bench`, `--machine` | ✓ VERIFIED | Run by hand above and by the `runner.*` tests. |
| `libretro/libretro.c` | core that delivers the card to a frontend | ✓ VERIFIED | Wired to `cogmaster_get_info` and `cogmaster_get_frame`. The in-process host and real RetroArch both match the pin. |
| `.github/workflows/ci.yml` | six legs plus be32, wasm32, retroarch, perf, rehearsal, cleanup, CI required | ✓ VERIFIED | Three green hosted runs. |
| `.github/workflows/release.yml` and the release-please config | publish on merge | ✓ VERIFIED (static) | Schema-valid. Runs on `main` only, so its first live run is at ship. |
| `README.md` install line | checksum-gated install | ✓ VERIFIED | Run in CI, with its negative control. |
| `docs/testing.md`, `docs/maintaining.md` | maintainer docs | ✓ VERIFIED | Present. `docs.budgets` passes. |

All artifacts across the 12 plans pass `gsd-tools verify.artifacts`, 37/37.

### Key Link Verification

All 23 key links across the 12 plans pass (`verify.key-links`, exit 0 for each plan). The links that matter most for the goal:

| From | To | Via | Status |
|------|----|-----|--------|
| runner, libretro host test, RetroArch job | `tests/testcard_pins.cmake` | same pinned SHA-256 | WIRED |
| build legs | hash-equality | `hashes-*` artifacts, COUNT=7 | WIRED (log: 7 identical) |
| build legs | rehearsal / release publish | `archives-*` artifacts and `commit.txt` | WIRED (log: 18 archives) |
| macos-arm64 leg | retroarch job | `archives-macos-arm64` | WIRED |
| `release.yml` | `ci.yml` | `workflow_call` with `ref` set to the release-please sha | WIRED (schema-valid; runs on main at ship) |

### Data-Flow Trace (Level 4)

| Artifact | Data | Source | Real data | Status |
|----------|------|--------|-----------|--------|
| libretro video callback | frame | `cogmaster_get_frame`, which takes it from `cogmaster__testcard_pixel` | yes; it hashes to the independently computed pin | ✓ FLOWING |
| RetroArch screenshot | frame | the built dylib inside RetroArch 1.22.2 | yes; it equals the runner PNG pixel for pixel | ✓ FLOWING |
| release assets | zips | the six build legs' merge-commit artifacts | yes; SHA256SUMS and attestations verified | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Full local CI | `cmake --workflow --preset ci` (from an empty `build/ci`) | exit 0; 80/80 and 4/4 passed | ✓ PASS |
| SMS hash | `build/ci/cogmaster-run --frames 60 --hash` | `e14d77ce…7ae7`, exit 0 | ✓ PASS |
| GG hash | `build/ci/cogmaster-run --machine gg --frames 60 --hash` | `7404ed4a…e779`, exit 0 | ✓ PASS |
| Independent card | `python3 -I card.py` (a version written from the card rules) | same two hashes; 4,096 GG colours | ✓ PASS |
| Bench | `build/ci/cogmaster-run --bench 600` | prints p50…p99.999, max and max_rss_kib | ✓ PASS |
| Signal and PNG | `build/ci/tests/cogmaster-test-run-stop --frames 60 --png 3:… --png 60:… --hash` | exit 0; frame-3 PNG equals the pinned PNG; no frame-60 file | ✓ PASS |
| Workflow schema | `uvx check-jsonschema --builtin-schema vendor.github-workflows .github/workflows/*.yml` | ok | ✓ PASS |
| Hosted CI | `gh run view 39335167905 / 39336389305 / 39337625991 --json conclusion` | success ×3 (bc256f2, a43ecfa, 79b6f19) | ✓ PASS |

Hosted runs:
- https://github.com/szTheory/cogmaster/actions/runs/39335167905
- https://github.com/szTheory/cogmaster/actions/runs/39336389305
- https://github.com/szTheory/cogmaster/actions/runs/39337625991

The commits after 79b6f19 change only `.planning/`, `docs/maintaining.md` and `docs/testing.md`. The local run covers
`docs.budgets` for them, and the phase PR will run hosted CI on them.

### Probe Execution

None declared. The project has no `scripts/*/tests/probe-*.sh`.

### Requirements Coverage

| Requirement | Source Plan(s) | Status | Evidence |
|-------------|----------------|--------|----------|
| FRAME-01 | 01, 07, 08, 09, 11 | ✓ SATISFIED | Truth 1 |
| FRAME-02 | 01, 02, 03, 04, 08, 09, 11 | ✓ SATISFIED | Truth 2 |
| FRAME-03 | 05 | ✓ SATISFIED | Truth 3 |
| FRAME-04 | 06 and others | ✓ SATISFIED | Truth 6 |
| FRAME-05 | 08, 09 and others | ✓ SATISFIED | Truth 7 |
| FRAME-06 | 07, 10 | ✓ SATISFIED | Truths 4 and 10 (the real publish is confirmed at ship, as the requirement says) |
| FRAME-07 | 07, 09 | ✓ SATISFIED | Truth 5 |
| FRAME-08 | 07 | ✓ SATISFIED | Truth 8 |
| FRAME-09 | 04, 12 | ✓ SATISFIED | Truth 9 |

Every phase 1 ID in REQUIREMENTS.md is claimed by at least one plan. No requirement is orphaned.

### Prohibitions (judgment tier, judged by the agent under project rule 6)

| Prohibition | Verdict | Evidence |
|-------------|---------|----------|
| No GPL/LGPL/NC/unlicensed code; vendored files carry notice and digest | Holds | `git grep` finds no GPL or LGPL text outside `.planning`. `libretro.h` is MIT ("Permission is hereby granted"), has its THIRD-PARTY-NOTICES section, and its digest is checked by `libretro.vendored`. |
| No BIOS, game or save bytes in the tree or archives | Holds | `file --mime-encoding` reports no tracked binary file. The card is generated by code. The archive contents are listed above. |
| No install without a verified checksum | Holds | The README runs `shasum -a 256 -c … && tar`. In CI the flip control exited 1 and installed nothing. |
| No personal path, e-mail or name in tree or archives | Holds | `hygiene.tree` is green. A grep of the three unpacked local archives for `/Users/`, `/home/runner` and the local user name finds nothing. |
| No non-prerelease or latest release from a PR | Holds | The rehearsal patches `prerelease=true`, `make_latest=false`. `policy.publish_signer` and `policy.publish_ref` pass. No release exists now. |

### Anti-Patterns Found

No `TBD`, `FIXME`, `XXX`, `TODO`, `HACK` or placeholder markers appear in the 123 non-planning files changed on this
branch (I left out the vendored `libretro.h`).

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `.github/workflows/ci.yml` and `scripts/classify-changes.sh` | ci.yml:76-78 | Review WR-02. When the PR file listing fails, the workflow feeds the classifier a `(listing failed)` sentinel. The classifier counts it as one path, so the output is `release_inputs=false`. Reproduced: `echo '(listing failed)' \| sh scripts/classify-changes.sh 5` prints `heavy=true` / `release_inputs=false`. The plan 08 truth holds at the script level, where empty input gives true, but the workflow's sentinel defeats its intent ("a change that cannot be seen rehearses"). | ⚠️ Warning | Only on a GitHub API failure, and only for a PR that changes a release input: that PR could merge without a rehearsal. Rule 7 makes it a tooling seed, not a goal blocker. |
| `src/internal.h` / `src/frame.c` | | Review WR-01. The docs promise zero-fill for short input structs, but the core dereferences the caller's struct at its own layout. | ℹ️ Info | Has no effect today, because the first-released size equals the current size. It becomes an out-of-bounds risk once the first field is appended. |
| `.github/workflows/ci.yml` | rehearsal | Review WR-04. The App key is usable from any branch in this repository. | ℹ️ Info | A security posture point for the owner. It does not affect the phase goal. |

### Human Verification Required

None. The only open owner step is applying the branch ruleset (`scripts/apply-repo-settings.sh`, handed over in the
plan 12 SUMMARY). It needs admin rights, so it is identity-bound under rule 6. It is a gate before ship, not a check of
this phase's goal: no must-have needs it applied, and `gh api repos/szTheory/cogmaster/rulesets` currently returns
none. The real publish on merge is confirmed at ship under rules 9 and 10.

### Gaps Summary

There are no gaps. The library, the runner, the in-process libretro host and RetroArch 1.22.2 all produce the same
test-card frame. That frame matches a hash computed independently from the card rules, and it is byte-identical on six
platforms and be32, with wasm32 equal as well. The README line installs the checksum-verified core where RetroArch
finds it. The PR rehearsal ran every publish step with the App token and cleaned up after itself. Review warnings
WR-01 to WR-04 are still open in `01-REVIEW-DISPOSITION.md`; none breaks a must-have, and WR-03 is answered above.

---

_Verified: 2026-10-19T23:47:27Z_
_Verifier: Claude (gsd-verifier)_
