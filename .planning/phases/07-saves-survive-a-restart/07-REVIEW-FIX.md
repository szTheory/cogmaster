---
phase: 07-saves-survive-a-restart
fixed_at: 2026-11-10T00:00:00Z
review_path: .planning/phases/07-saves-survive-a-restart/07-REVIEW.md
iteration: 1
findings_in_scope: 2
fixed: 2
skipped: 0
status: all_fixed
---

# Phase 07: Code Review Fix Report

**Fixed at:** 2026-11-10
**Source review:** .planning/phases/07-saves-survive-a-restart/07-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 2 (Warnings WR-01 and WR-02)
- Fixed: 2
- Skipped: 0
- Out of scope: IN-01 and IN-02 stay in REVIEW.md; they are tooling and CLI polish and become seeds (rule 7).

## Fixed Issues

### WR-01: `eeprom_io` does not cross-check the loaded chip fields against each other

**Files modified:** `src/state.c`, `tests/mapper/test_eeprom.c`
**Commits:** a67a3d9, d9625cb (clang-format of the new lines)
**Status:** fixed: requires human verification (state-validity logic)
**Applied fix:** `eeprom_io` now also refuses a state with Data Out low outside a read, a read whose command
is not READ, a data-in phase whose command is neither WRITE nor WRAL, and a waiting program (flag b4) unless
the phase is command complete, writes are enabled and the command is WRITE, ERASE, WRAL or ERAL. The checks use
the values the loader returns for each field: in a load the instance fields are staged, so reading `e->cmd`
there saw the old command (the first attempt, which read `e->cmd`, failed `eeprom.state.mid_command`). The
existing `eeprom.state.bounds` case now crafts six refusals: a waiting program in an idle phase, Data Out low
in data in, a READ command in data in, a WRITE command in a read, a waiting program after EWDS (writes off)
and after EWEN (nothing to program). No new test name, no inventory change.

### WR-02: A failed `open` of the directory in `save_sync_directory` reports a flush failure after the save is in place

**Files modified:** `runner/save.c`, `docs/runner.md`, `tests/runner/test_save.c`
**Commit:** c66aaba
**Applied fix:** `save_sync_directory` returns success when `open` of the directory fails with `EACCES` or
`EPERM`, with a comment citing POSIX.1 `open()`; the injected `dirsync` fault still returns failure before the
open, so `runner.save.process.dirsync` and the `dirsync` stage of `runner.save.failures` are unchanged.
`docs/runner.md` states the rule next to the `EINVAL` and `ENOTSUP` text. A regression case runs at the end of
the existing `runner.save.failures` test (no new name): a mode 0300 directory takes a flush and acknowledges it
(for a privileged user the flush is ordinary and the case still holds).

## Verification

Run in the main checkout (`workflow.use_worktrees` is false), from the repository root:

| Command | Exit |
|---|---|
| `cmake --workflow --preset ci` | 0 |
| `cmake --workflow --preset hygiene` | 8 on its own run: `hygiene.format` (fixed in d9625cb) and `hygiene.tree` at the 60 s limit |
| `sh scripts/hygiene.sh --tree` (what `hygiene.tree` runs, outside the 60 s limit; 25 s) | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (723 check lines / 8279 src lines) |

After d9625cb the hygiene workflow's only remaining failure was the `hygiene.tree` timeout.

Negative controls:
- WR-01: with the four new checks removed from `eeprom_io`, `eeprom.state.bounds` fails; restored, it passes.
- WR-02: with `runner/save.c` reverted, `runner.save.failures` fails; restored, `runner.save` passes (29 of 29).

---

_Fixed: 2026-11-10_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
