---
phase: 03-games-play-in-retroarch
fixed_at: 2026-10-20T23:20:42.500Z
review_path: .planning/phases/03-games-play-in-retroarch/03-REVIEW.md
iteration: 1
findings_in_scope: 2
fixed: 2
skipped: 0
status: all_fixed
---

# Phase 03: Code Review Fix Report

Both findings from the existing standard review were fixed inline in Codex (GPT-6),
using the configured main checkout without a worktree. This is fix round 1 of 2;
no automatic re-review loop or phase verification was run.

## Fixed Issues

### CR-01: Windows PNG failure cleanup can delete a different Unicode filename

**Commit:** e826b09
**Files modified:** `runner/path.h`, `runner/main.c`, `tests/runner/tests.cmake`,
`tests/runner/png_close_failure.c` (new).

PNG failure cleanup now uses the same UTF-8-to-UTF-16 conversion as opening the
output, calling `_wremove` on Windows and `remove` on POSIX. The existing
`runner.png_file` test also runs production runner code with an injected close
failure. It checks that the intended Unicode output disappears and a distinct
CP1252-decoded filename retains its original bytes. On Windows the test selects
CRT code page 1252 immediately before cleanup; production has no injection hook.
The new test wrapper replaces `fclose` only in its included runner translation unit.

The native Windows branch is registered in CI but was not executed locally on
macOS. Local execution proves failed-close cleanup and preservation of the distinct
filename; Windows execution remains part of the normal hosted matrix.

### WR-01: The runner prints the new VDP fault as two unknown fields

**Commit:** 060da4a
**Files modified:** `runner/main.c`, `docs/runner.md`, `tests/runner/tests.cmake`.

The kind and device tables use designated public constant indexes and include
`unsupported_video_mode` and `vdp`. The runner guide lists both names. The existing
`runner.trace.fault` test creates a synthetic 1 KiB display-enabled legacy-mode
image in the build tree, checks successful exit and the symbolic VDP fault output,
then removes the image. Existing unmapped-read and no-fault checks remain intact.

## Verification

Commands run from the repository root in the main checkout. Run URL: not applicable
for local checks. No logs are committed. An initial hygiene run found only a
clang-format mismatch; formatting was corrected in the WR-01 commit before the
final checks below. Formal phase verification remains the next GSD step.

| Command | Exit status | Run URL |
|---|---:|---|
| `cmake --workflow --preset ci` | 0 | N/A (local) |
| `cmake --preset hygiene` | 0 | N/A (local) |
| `cmake --build --preset hygiene` | 0 | N/A (local) |
| `ctest --preset hygiene` | 0 | N/A (local) |
| `git diff --check` | 0 | N/A (local) |
