---
phase: 13-why-carts-fail
verified: 2026-11-12T19:50:00Z
status: passed
score: 4/4 must-haves verified
covered_files:
  - ".planning/phases/13-why-carts-fail/13-01-PLAN.md"
  - ".planning/phases/13-why-carts-fail/13-01-SUMMARY.md"
  - ".planning/phases/13-why-carts-fail/13-02-PLAN.md"
  - ".planning/phases/13-why-carts-fail/13-02-SUMMARY.md"
  - ".planning/phases/13-why-carts-fail/13-03-PLAN.md"
  - ".planning/phases/13-why-carts-fail/13-03-SUMMARY.md"
  - ".planning/phases/13-why-carts-fail/13-04-PLAN.md"
  - ".planning/phases/13-why-carts-fail/13-04-SUMMARY.md"
  - ".planning/phases/13-why-carts-fail/13-05-PLAN.md"
  - ".planning/phases/13-why-carts-fail/13-05-SUMMARY.md"
  - "runner/diag.c"
  - "runner/diag.h"
  - "runner/diag_class.h"
  - "runner/main.c"
  - "scripts/library.cmake"
  - "scripts/library.sh"
  - "tests/library/diagnose.cmake"
  - "tests/runner/diagnose.cmake"
covered_digest: "v4:sha256:57198400ec7b53f2cedeacc2e0b6786a86cd0d9442d0098cae2bbe0692117505"
behavior_unverified: 0
overrides_applied: 0
advisory:
  - finding: "WR-02: ram=live|still can read the last bus byte when a guest turns work RAM off in port $3E, so the frozen versus alive_still split can count noise"
    category: other
    reason: "Deliberately left open in 13-REVIEW-DISPOSITION.md as an owner decision (fix in Phase 14 or accept). Only the static_no_fault split reads ram; in the owner run that is 1 of 205 failing rows, and no fault-kind group (ranks 1-8) depends on it."
    evidence_status: "code read (runner/diag.c, scripts/library.sh line 148) and the review's analysis; not exercised by a test"
---

# Phase 13: Why carts fail Verification Report

**Phase Goal:** The failing default-profile rows have a known cause, grouped by size, from a diagnostic that leaks nothing from the owner's dumps. No fix is made in this phase and no ledger row moves.
**Verified:** 2026-11-12
**Status:** passed
**Re-verification:** No, initial verification

## Goal Achievement

### Observable Truths (ROADMAP success criteria)

| # | Truth | Status | Evidence |
| - | ----- | ------ | -------- |
| 1 | The library judge in diagnostic mode prints, per default-profile `fail` row, the frames to first non-blank sample, the first fault kind and PC, and the mapper chosen, and nothing but counts, kinds and catalog titles | VERIFIED | `scripts/library.sh` diagnose branch builds a `diag:` line from validated fields only (`first_display`, `first_nonblank`, `distinct600/1800`, `fault`, `dev`, `pc`, `mapper`, plus `first_change`, `fault_frame`, `order`, `where`, `ram`, `source`), each matched against a closed pattern and printed with printf (library.sh lines 121-160). Titles pass a charset and 80-byte gate, else `title=-`. `ctest --preset ci -R diagnose` passes `library.diagnose`, `.taxonomy`, `.hostile`, `runner.diagnose`, `runner.diagnose.class`. |
| 2 | A synthetic-fixture test shows the output holds no image bytes, file names or paths, and a negative control that leaks one makes the same test fail | VERIFIED | `tests/library/diagnose.cmake` plants canaries (bytes, name, home-path fragment, directory) and scans stdout, stderr, `diagnose.txt` and every file under the output directory. `library.diagnose.leak_name`, `.leak_path`, `.leak_bytes`, `.leak_digest` and `.leak_title` are registered through `tests/cmake/expect_fail.cmake` with `CODE=1` and a `diagnose leak: leak_<class>` regex; all five pass, so the same scan does fail when a leak is planted. `library.diagnose.hostile` also passes. |
| 3 | A planning note records the failing rows grouped by cause with a count per group, counts only, in descending order, citable by the next phase | VERIFIED | `13-CAUSE-GROUPS.md` has a header (core commit 470b058, which exists in git; behaviour revision 17, matching `include/cogmaster.h`; ledger commit 032ebab, which exists; command and exit status 0; caps 600 and 1800). The table has 11 ranks, rows 109, 36, 32, 8, 7, 6, 4, 1, 1, 1, 0, in descending order and summing to 205, which equals `fail_total` and `groups_total` in the totals block. "groups under 5: 5 groups, 7 rows" agrees with ranks 7 to 11. The mapper-by-class cross-cut also sums to 205. The note has no hash, path, e-mail address or title. Only runner/diag.h comments and library.sh traps changed after the 470b058 run, so the note is not stale for the fault-kind groups. |
| 4 | `cmake --workflow --preset ci` passes and the committed ledger and compatibility page are unchanged | VERIFIED | `git diff --exit-code origin/main -- tests/compat/ledger.csv docs/compatibility.md` exits 0, and `git diff --quiet origin/main -- include` exits 0. Hosted CI run 43334476928 on head 27d3e30 passed (as reported by the caller; later commits touch docs, comments, traps and planning files only). Locally: the 13 diagnose, `runner.expect.*` and `runner.stop` tests, all 98 `runner.*` plus `library.synthetic` tests, and `docs.budgets*`, `docs.api_names*` and `policy.e2e_budget*` pass. The full workflow was not run because of host load. |

**Score:** 4/4 truths verified, 0 behavior-unverified. The diagnostic is an offline counts tool, so no truth is a runtime state-transition invariant that lacks a test.

### Required Artifacts

| Artifact | Expected | Status | Details |
| -------- | -------- | ------ | ------- |
| `runner/diag.c`, `runner/diag.h`, `runner/diag_class.h` | `--diagnose` state, per-frame step, one-line print, pure class helpers | VERIFIED | Present (136 lines in diag.c) and wired from `runner/main.c`; `runner.diagnose` and `runner.diagnose.class` pass. |
| `scripts/library.sh`, `scripts/library.cmake` | diagnose branch, validator, reducer, early `COGMASTER_DIAGNOSE` block | VERIFIED | Present and exercised end to end by `library.diagnose*`. The only new `COGMASTER_` names in the diff are `COGMASTER_DIAGNOSE` and names `run` already used (`COGMASTER_IMAGE`, `COGMASTER_LIBRARY_RUN`, `COGMASTER_LIBRARY_OUT`, `COGMASTER_CATALOG`, `COGMASTER_ROM_DIR`), and `COGMASTER_BEHAVIOUR_REVISION` is a header macro read by awk. Each new gate carries a `# Fault prevented:` comment (lines 12, 53, 71). |
| `tests/library/diagnose.cmake`, `tests/runner/diagnose.cmake` | canary scan, controls, taxonomy, shape tests | VERIFIED | Registered in `tests/library/tests.cmake` and `tests/runner/tests.cmake`; 10 diagnose tests run and pass. |
| `13-CAUSE-GROUPS.md` | counts-only cause table | VERIFIED | See truth 3. |
| `docs/runner.md`, `docs/testing.md`, `docs/conformance.md`, `ASSET_POLICY.md` | docs of the mode and policy | VERIFIED | All under their caps (runner.md 36,916 of 38,912 bytes); `docs.budgets` passes; README unchanged. |

### Key Link Verification

| From | To | Via | Status |
| ---- | -- | --- | ------ |
| `runner/diag.c` | `runner/expect.c` | shared `cogmaster__expect_boots` verdict at frame 600 | WIRED (`runner.diagnose` pins verdict equality with `--expect boots@600`; `runner.expect.predicates` still passes) |
| `scripts/library.sh` diagnose | `scripts/library.cmake` | `COGMASTER_DIAGNOSE` records file | WIRED |
| `scripts/library.cmake` | `cogmaster-run --diagnose` | per-image call | WIRED |
| `scripts/library.sh` awk | `compat-common.sh cases` | ledger read for cross-check totals | WIRED (the ledger is not split on commas; run_fail 205, ledger_fail 209, matched 205 in the note) |
| diagnose branch | ledger and page merge block | must not reach it | VERIFIED (the merge block that writes `$ledger` and `$page` is at lines 234-247, after the diagnose branch's exit; `library.diagnose` hashes the stand-in ledger and page before and after) |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| ----------- | ----------- | ----------- | ------ | -------- |
| DIAG-01 | 13-01, 13-02, 13-03 | diagnostic mode reporting frames to first non-blank, first fault kind and PC, mapper, counts only | SATISFIED | Truth 1; marked complete in REQUIREMENTS.md |
| DIAG-02 | 13-04 | synthetic-fixture leak test plus a negative control that makes it fail | SATISFIED | Truth 2 |
| DIAG-03 | 13-05 | failing rows grouped by cause with counts, recorded in a planning note | SATISFIED | Truth 3 |

All three IDs in the plan frontmatter match the phase's REQUIREMENTS.md mapping. No orphaned requirements.

### Prohibitions

| Prohibition | Verification | Status |
| ----------- | ------------ | ------ |
| The `--diagnose` line never prints a digest, opcode, exact fault frame, image size, file name or path | `runner.diagnose` anchored line check | VERIFIED (test passes) |
| No public header, ABI version or behaviour revision change | `git diff --quiet origin/main -- include` | VERIFIED (exit 0) |
| diagnose never writes the ledger or page | `library.diagnose` stand-in hashes; merge block unreachable from the branch | VERIFIED |
| No leak or test switch in library.sh, library.cmake or the runner | grep of added `COGMASTER_` names | VERIFIED |
| No whole canary, home path or e-mail in a tracked file | canaries assembled with `string(CONCAT)`; grep of the note and `docs/runner.md` for home paths and e-mail found none; hygiene was green in the hosted CI run | VERIFIED |

### Anti-Patterns Found

No unreferenced `TBD`, `FIXME` or `XXX` markers were found as blockers in the changed files. Review findings WR-01 and WR-03 are fixed (2e9a44b, 0e6e5bf). IN-01, IN-02, IN-03, IN-05 and seeds SD-01 to SD-04 are open as info or seeds by the project's rule 7 and are not blockers. SD-04 notes `tests/library/diagnose.cmake` at 301 lines against the plan's 221 cap, with no gate that counts it.

### Advisory (WR-02, decision left open)

`ram=live|still` may read open bus rather than work RAM when the guest disables RAM in port $3E. The `ram` field feeds only the `static_no_fault` split between `frozen` and `alive_still`. In the owner run that group holds one failing row (`static_no_fault.frozen`) out of 205, and no fault-kind group (ranks 1 to 8) depends on it. The phase goal, a known cause grouped by size, holds either way. The disposition leaves the fix to Phase 14 or acceptance, and fixing it needs another owner diagnose run, so it is recorded here and not as a gap.

### Human Verification Required

None. Per project rule 6, every check above is a command and an exit status.

### Gaps Summary

No gaps. The diagnostic exists, is wired, passes its tests (including five negative controls that fail on a planted leak), and the owner-run cause note is counts-only, ordered, and reconciled to the ledger (205 failing rows run, 209 in the ledger, 4 ledger fails not in the run, 0 run fails missing from the ledger). No ledger row or page changed, and no public header changed.

---

_Verified: 2026-11-12_
_Verifier: Claude (gsd-verifier)_
