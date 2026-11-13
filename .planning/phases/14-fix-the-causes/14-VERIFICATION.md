---
phase: 14-fix-the-causes
verified: 2026-11-13T00:00:00Z
status: human_needed
score: 5/6 must-haves verified
covered_files:
  - ".planning/phases/14-fix-the-causes/14-01-PLAN.md"
  - ".planning/phases/14-fix-the-causes/14-01-SUMMARY.md"
  - ".planning/phases/14-fix-the-causes/14-02-PLAN.md"
  - ".planning/phases/14-fix-the-causes/14-02-SUMMARY.md"
  - ".planning/phases/14-fix-the-causes/14-03-PLAN.md"
  - ".planning/phases/14-fix-the-causes/14-03-SUMMARY.md"
  - ".planning/phases/14-fix-the-causes/14-04-PLAN.md"
  - ".planning/phases/14-fix-the-causes/14-04-SUMMARY.md"
  - ".planning/phases/14-fix-the-causes/14-05-PLAN.md"
  - ".planning/phases/14-fix-the-causes/14-05-SUMMARY.md"
  - ".planning/phases/14-fix-the-causes/14-06-PLAN.md"
  - ".planning/phases/14-fix-the-causes/14-06-SUMMARY.md"
  - ".planning/phases/14-fix-the-causes/14-07-PLAN.md"
  - ".planning/phases/14-fix-the-causes/14-07-SUMMARY.md"
  - ".planning/phases/14-fix-the-causes/14-08-PLAN.md"
  - ".planning/phases/14-fix-the-causes/14-08-SUMMARY.md"
  - "docs/embedding.md"
  - "include/cogmaster.h"
  - "runner/diag.c"
  - "runner/diag_class.h"
  - "src/io.c"
  - "src/mapper.c"
  - "src/mapper_ext.c"
  - "src/vdp_render.c"
  - "tests/bus/test_bus.c"
  - "tests/mapper/test_mapper_ext.c"
  - "tests/runner/test_runner.c"
  - "tests/vdp/test_background.c"
covered_digest: "v4:sha256:38f1eff393545477f930d7dd275c80a80066865540166fc92bec01d3765a9de4"
behavior_unverified: 0
overrides_applied: 0
human_verification:
  - test: "Take the one D-14 pad-press run for the single static_no_fault row (button 1 on a machine sms row) and, if wanted, the D-06 trace of the Sega bank shift on one former $FFFC cart"
    expected: "Pad press: the picture does not change inside the frame cap (rank 10 stays closed). Trace: bits 1-0 of the written $FFFC value are clear in the common case (shift left as it is). Alternatively accept both as deferred to Phase 15's confirmation batch by recording an override"
    why_human: "Both need the owner's dump directory to find the cart; the agent may not read dump names or paths (CLAUDE.md rule 6 allows asking only for a run on the owner's own data)"
---

# Phase 14: Fix the causes Verification Report

**Phase Goal:** Every cause group of 5 or more failing rows is fixed in the core or the boots check, or closed with a stated reason, so carts that run correctly stop failing for the wrong reason.
**Verified:** 2026-11-13
**Status:** human_needed (the goal is achieved; one plan-level must-have, the pad-press run, was not taken and needs an owner decision to accept or run)
**Re-verification:** No, initial verification

The code-review step was skipped (denied by the auto-mode permission classifier); this is a skipped advisory step, not a pass.

## Goal Achievement

### Observable Truths

| #   | Truth | Status | Evidence |
| --- | ----- | ------ | -------- |
| 1 | SC1: each group of 5 or more rows (ranks 1-6) has a recorded outcome, fixed or closed with reason, in rank order, citing the Phase 13 note | VERIFIED | `14-FIX-OUTCOMES.md` rows for ranks 1-11. Baseline leaf sums reconcile: 109+36+32+8+7+6+4+1+1+1 = 205; head leaves 40+6+2+1 = 49. Ranks 1, 3, 4, 5, 7, 8 fixed by code (below); ranks 2 and 6 closed "not emulated by design" with source (SMS-VDP 7, DEC.03, verified present in DECISIONS.md), why fail is honest, reopen condition; ranks 9-11 closed "no evidence" with four parts |
| 2 | SC2: boots check sound, stated reason, never-draws fixture stays | VERIFIED | Outcomes note "Boots check" section: 3 no-fault rows, no leaf of 5, 0 late-picture rows over 49 failing rows. `git diff --quiet origin/main -- runner/expect.c scripts/compat-common.sh scripts/library.cmake` exit 0. `ctest -R '^(runner.expect.predicates\|runner.diagnose\|runner.hashes_file\|runner.hashes_compare.*)$'` 5/5 pass (boots@20 pass, static seed fails, no-image fails) |
| 3 | SC3: zemina, korean_nb, korean each have a cause and a closed outcome | VERIFIED | "Families" table, in the required order, each with cause, outcome, class, source, why fail is honest, reopen. All three are the legacy TMS mode cause (8, 5, 2 rows), no M4=1 row, closed per D-07. The 2 korean alive rows counted once as passing |
| 4 | SC4: every core fix has a ci test; moved pins named; revision bumped | VERIFIED | `COGMASTER_BEHAVIOUR_REVISION` 17 to 18 in `include/cogmaster.h`; `tests/core/test_core.c` asserts 18. Each dropped raise site has a flipped test (test_bus.c, test_mapper_ext.c, test_io.c, test_background.c now asserts `f.addr == mode` and no fault on supported modes). No pin moved: `runner.hashes_file` passes and the base-versus-head compare is recorded exit 0 |
| 5 | SC5: `cmake --workflow --preset ci` passes at head, no pass row's fixture changes without a pin | VERIFIED | Orchestrator's gate: exit 0, 676/676 + 5/5 at f37dc52. Independently, this run: `ctest -R '^(bus\|mapper\|io\|vdp\|api\|core\|docs)'` 140/140 pass. Fences: ledger, docs/compatibility.md, ci.yml unchanged vs origin/main (exit 0); no TIMEOUT line changed |
| 6 | Plan 08 truth: `static_no_fault` gets exactly one pad-press run (D-14) | UNCERTAIN | Not taken; recorded with a reason in rank 10 and the Boots check section. Rank 10 is closed on its own `distinct1800` of 1, and "reopens if a pad press changes the picture" is stated. See Human Verification |

**Score:** 5/6 truths verified (0 behavior-unverified). One must-have (a plan-level, not roadmap-level one) is UNCERTAIN.

### Code evidence for the fixes (read from the diff, not the summaries)

| Group | Change | Verified in |
| ----- | ------ | ----------- |
| Rank 1, 8 | `$FFFC` unused-bit raise and `SEGA_UNUSED` removed; `p->ctrl = v` kept; bank-past-end fault kept | `src/mapper.c` |
| Rank 3, 7 | every `rom_write` raise site removed on Sega, Codemasters, Korean, Hicom, Zemina, 4PAK, EEPROM and the default board | `src/mapper.c`, `src/mapper_ext.c` |
| Rank 4, 5 | the `unmapped_read` of `$00-$3F` and the `unmapped_write` of undecoded ports removed; memory-side faults stay | `src/io.c` |
| Rank 2, 6 | no renderer by design; the fault now records the mode nibble (`vdp_mode()`), which separates a legacy refusal from a decode bug | `src/vdp_render.c`, `tests/vdp/test_background.c` |
| Evidence words | closed-vocabulary diagnose classes, tested by `runner.diagnose` and `test_runner.c` | `runner/diag_class.h`, `runner/diag.c` |

### Requirements Coverage

| Requirement | Source Plans | Status | Evidence |
| ----------- | ------------ | ------ | -------- |
| FIX-01 | 01, 02, 03, 04, 05, 06, 07, 08 | SATISFIED on its wording | Every group of 5+ is fixed (ranks 1, 3, 4, 5, 8) or closed with a stated reason (ranks 2, 6); descending order held in the note |
| FIX-02 | 08 | SATISFIED | Check recorded sound with stated reasons; predicate fixture stays and passes |
| FIX-03 | 01, 04, 08 | SATISFIED on its wording | Three family rows with cause and closed reason, no `open: D-10` cell |
| FIX-04 | 01-08 | SATISFIED | Revision 18, tests, pin compare exit 0, ci green |

No orphaned requirement: REQUIREMENTS.md maps exactly FIX-01 to FIX-04 to Phase 14 and every ID appears in at least one plan's frontmatter.

**Judgement on the FIX-01 and FIX-03 status (asked by the orchestrator):** plan 08 left both open only because its own truth 2 says to while any `open:` cell exists. The requirement text does not support that. FIX-01 asks that each group be fixed or closed with a reason. Rank 3 is fixed (the leaf prints 0 at the head, `run_fail_not_in_ledger` 0). Its `open: D-04` cell is a follow-up question: whether the Sega board is the right choice for the carts that write to the Codemasters register `$0000` (11 to 12 rows at baseline, all board source db, so neither a header rule nor a table row can move them). That is a seed (SEED-016), not a failing group. FIX-03 has no open family cell at all; the `open:` on rank 3 does not touch it. A gap round with no new evidence is what CLAUDE.md rule 7 says not to run. Recommendation: mark FIX-01 and FIX-03 complete in REQUIREMENTS.md (the checkbox and the traceability table). This is a recommendation, not an edit; this verifier does not commit or change requirements.

### Not-taken items check (D-06 trace, pad press)

- D-06 trace: plan 03's own Task 2 branches on "yes, no or not taken" and says for "not taken" make no code edit and record why. That branch was followed: recorded in the rank 1 evidence, plan 03's SUMMARY and the Gap round "not taken" list, with the reason (needs a dump path). No roadmap criterion or must-have depends on it. Residual risk, honestly stated in plan 03's SUMMARY: 9 of the 110 rows moved out of ranks 1 and 8 landed on later faults, which a bank shift could also produce; the per-cart link was never traced.
- D-10 mapper traces: not taken because no family row is M4=1; nothing to ask them. Sound.
- Pad press: a plan 08 must-have (truth 6) and D-14. Recorded with a reason in rank 10, Boots check, SUMMARY deviations. It is the only plan-level must-have not met, hence UNCERTAIN.

### Anti-Patterns Found

None. A scan of the lines added by the phase in `src`, `runner`, `tests`, `docs`, `include` for TBD, FIXME, XXX, TODO, HACK found no match. No ROM or tests/roms changes in the diff.

### Probe Execution

SKIPPED: no probe-based verification declared in the phase plans.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| -------- | ------- | ------ | ------ |
| Boots predicate boundary, diagnose, pins | `ctest --test-dir build/ci -R '^(runner\.expect\.predicates\|runner\.diagnose\|runner\.hashes_file\|runner\.hashes_compare.*)$' --no-tests=error` | 5/5 pass | PASS |
| Fault-policy groups | `ctest --test-dir build/ci -R '^(bus\|mapper\|io\|vdp\|api\|core\|docs)' -j8` | 140/140 pass | PASS |
| Check and ledger fences | `git diff --quiet origin/main -- runner/expect.c scripts/compat-common.sh scripts/library.cmake` and `git diff --exit-code --quiet origin/main -- tests/compat/ledger.csv docs/compatibility.md .github/workflows/ci.yml` | exit 0, exit 0 | PASS |

### Evidence I could not reproduce

The per-leaf counts (205 to 49, 40/6/2/1 at the head, 0 late-picture rows) come from `sh scripts/library.sh diagnose` over the owner's dump set. I did not re-run it: it needs `COGMASTER_ROM_DIR`, which I may not read, and it prints catalog titles. I checked the numbers only for internal consistency (the sums above, the family counts against the vdpmode sub-rows, the baseline class table against rank totals). The outcome rows cite the run command, exit status and head commit, as CLAUDE.md rule 11 requires.

### Human Verification Required

### 1. Pad press for `static_no_fault` (and, optionally, the D-06 bank-shift trace)

**Test:** run the one pad-press run (button 1 on a machine sms row) on the single `static_no_fault` row, and the one `--trace first-fault` read of a former `$FFFC` cart, on the owner's dump directory.
**Expected:** the picture does not change inside the frame cap (rank 10 stays closed); bits 1-0 of the `$FFFC` value are clear in the common case. Or record an override accepting both as deferred to Phase 15's confirmation batch.
**Why human:** both need the dump directory to find a cart; the agent may not read it.

Suggested override if the owner accepts the deferral:

```yaml
overrides:
  - must_have: "static_no_fault gets exactly one pad-press run (exit status recorded only)"
    reason: "Needs the dump directory to find the cart; rank 10 is closed on its own distinct1800 and the run is queued for Phase 15's confirmation batch"
    accepted_by: "{owner}"
    accepted_at: "{ISO timestamp}"
```

### Gaps Summary

No blocking gap. The phase goal is met in the code: five groups fixed with flipped tests, two closed as not emulated by design with their source and reopen conditions, the boots check recorded sound with its predicate fixture intact, no pin moved, revision 18. The two unrun traces are recorded with reasons and cost nothing against the roadmap criteria.

---

_Verified: 2026-11-13_
_Verifier: Claude (gsd-verifier)_
