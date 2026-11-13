---
phase: 13-why-carts-fail
fixed_at: 2026-11-12T00:00:00Z
review_path: .planning/phases/13-why-carts-fail/13-REVIEW.md
iteration: 1
findings_in_scope: 2
fixed: 2
skipped: 0
status: all_fixed
---

# Phase 13: Code Review Fix Report

**Fixed at:** 2026-11-12
**Source review:** .planning/phases/13-why-carts-fail/13-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 2 (WR-01, WR-03; IN-04 folded into WR-03 as a few-line change)
- Fixed: 2
- Skipped: 0
- Deliberately not attempted: WR-02 (changes diagnose behaviour, deferred), IN-01, IN-02, IN-03, IN-05, seeds

Edits and commits were made in the main checkout on `phase/13-why-carts-fail` (`workflow.use_worktrees` is false).

## Fixed Issues

### WR-01: The diagnose line is documented as free of "exact frame" but prints three exact frame numbers

**Files modified:** `runner/diag.h`, `docs/runner.md`
**Commit:** 2e9a44b
**Applied fix:** The header comment and the Diagnose paragraph now say the line holds no digest, byte, image size, name or path, and that it does print the first-event call numbers and the fault pc while the fault's own frame is bucketed. "exact frame" and "region guess" are gone from the "never" list. Docs stay inside their byte budgets (`docs/runner.md` 36849 to about 36900 bytes; the budget check ran inside `ci`).

### WR-03: The scratch directory is removed only on a normal exit

**Files modified:** `scripts/library.sh`, `docs/testing.md`
**Commit:** 0e6e5bf
**Applied fix:** Added `trap 'exit 130' INT`, `trap 'exit 143' TERM` and `trap 'exit 129' HUP` after the unchanged EXIT trap (POSIX sh only). `docs/testing.md` now says the traps remove the directory on exit and on SIGINT, SIGTERM and SIGHUP, and that only SIGKILL leaves it behind. IN-04 was applied too: line 2 and the "Fault prevented" comment on the diagnose guard now describe what the code does, and the scratch comment mentions the signal traps. `sh -n scripts/library.sh` exits 0. Not exercised by running the script (instructed not to).

## Verification

Run in the main checkout, not a worktree, with both edits in the working tree.

- `cmake --workflow --preset ci`: first run exit 8 (`cart.probe.pure` timed out at 10.29 s while the machine load average was above 60; a test unrelated to these files); rerun exit 0, including the 16 `library.diagnose` entries.
- `cmake --workflow --preset hygiene`: exit 8 on every run, only `hygiene.tree (Timeout)` at 60 s, 83 of 84 tests pass. The same exit 8 and the same single timeout occur on the unmodified HEAD (changes stashed), so it is not caused by these fixes: the test takes about 50 s alone and exceeds its 60 s timeout under the current host load (`ctest --preset hygiene -R '^hygiene.tree$'` passed in 49.9 s and 51.9 s run alone with the fixes applied). Needs a rerun on an idle machine or in CI to confirm exit 0.

---

_Fixed: 2026-11-12_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
