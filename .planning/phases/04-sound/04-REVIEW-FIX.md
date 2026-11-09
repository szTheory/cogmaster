---
phase: 04-sound
fixed_at: 2026-11-09T00:00:00Z
review_path: .planning/phases/04-sound/04-REVIEW.md
iteration: 1
findings_in_scope: 2
fixed: 2
skipped: 0
status: all_fixed
---

# Phase 4: Code Review Fix Report

**Fixed at:** 2026-11-09
**Source review:** .planning/phases/04-sound/04-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 2
- Fixed: 2
- Skipped: 0

The four Info findings (IN-01 to IN-04) are out of scope for `critical_warning` and remain in REVIEW.md.
`workflow.use_worktrees` is false, so edits and commits were made in the main checkout on `phase/04-sound`.

## Fixed Issues

### WR-01: The trace log is not "in causal order", and its ticks do fall within one run

**Files modified:** `docs/audio.md`, `src/internal.h`
**Commit:** df8a27e
**Applied fix:** Docs-only resolution. Both places now state the real guarantee: records of one source are in tick
order and never go backwards; across sources the log is in the order the chips were brought up to date, so a
consumer sorts by tick, then source, then emission index, as `cogmaster-run --hash-audio` does (sort keys confirmed
in `runner/audio.c`). Emission order and every hash are unchanged. Status: fixed (documentation change; the
wording is worth a human read).

### WR-02: Impossible-state guard `if (m->frame > 0u)` in `cogmaster__synth_frame_end`

**Files modified:** `src/synth.c`
**Commit:** e1ac29f
**Applied fix:** Confirmed both callers run after the frame count is incremented (`bus.c:102` via
`cogmaster__bus_run_frame` for `frame.c:61`, and `frame.c:74` for `frame.c:79`), and that `m->frame` is `uint64_t`
(no wrap). Removed the `if`, kept the `m->frame--` / `m->frame++` pair and recorded the invariant in the comment.
Behaviour on every reachable path is identical. Status: fixed: requires human verification (the finding is a
necessity removal).

## Verification

Run from the repo root in the main checkout (not a worktree), with `/opt/homebrew/opt/llvm@21/bin` on PATH:

- `cmake --preset ci`, `cmake --build --preset ci`: exit 0
- `ctest --preset ci`: exit 0, 399 of 399 passed (audio pins, 66 playing rows and hash pins unchanged)
- `cmake --workflow --preset ci`: exit 0
- `cmake --workflow --preset hygiene`: exit 0

---

_Fixed: 2026-11-09_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
