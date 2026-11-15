# Phase 17 run evidence

Counts, commands, exit statuses and run URLs only. No library path, title or file name appears here.

## Pushed head

Head `1b8d4466ab72b4ca75e5c026270540a3f70165cc` on `phase/17-re-run-merge-and-clean-state` (code of plans 01 and 02 plus one fix to plan 02's script). Phase base `0a97075f34e48e2b1d0f72aa231006cc9366a398`.

| step | command | exit status |
|------|---------|-------------|
| format | clang-format 21 `--dry-run --Werror` over `git ls-files -z '*.c' '*.h' ':!:libretro/libretro.h'` | 0 |
| ci preset | `cmake --workflow --preset ci` | 0 |
| hygiene preset | `cmake --workflow --preset hygiene` | 0 |
| tree check | `sh scripts/hygiene.sh --tree` | 0 |
| push | `git push -u origin phase/17-re-run-merge-and-clean-state` | 0 |
| first dispatched run | `gh workflow run ci.yml --ref phase/17-re-run-merge-and-clean-state`, head e896d84, https://github.com/szTheory/cogmaster/actions/runs/43622165959 | failed: `policy.milestone_close` cases baseline, release_tag and pending on both Windows legs |
| fix | commit 1b8d446 passes `MSYS_NO_PATHCONV=1` and `MSYS2_ARG_CONV_EXCL='*'` to the one `git show origin/main:<path>` call in `scripts/milestone-close.sh`; `ctest --preset ci -R '^policy\.milestone_close$'` | 0 |
| second dispatched run | `gh run watch --exit-status`, head 1b8d446, https://github.com/szTheory/cogmaster/actions/runs/43657530824 | 0 (conclusion success, no failed job) |
| stamp control | `git merge-base --is-ancestor 0b4ae67b7f14c3d997c24ca719fb18f1d3cd29a4 origin/main` (Phase 15 pre-squash stamp) | 1 (not an ancestor, as the control requires) |
| base is ancestor | `git merge-base --is-ancestor 0a97075f34e48e2b1d0f72aa231006cc9366a398 origin/main` | 0 |
| sources equal | `git diff --quiet 0a97075 HEAD -- src include CMakeLists.txt cmake tests/compat/ledger.csv docs/compatibility.md tests/compat/allow.txt quality.json` | 0 |
| worktree head | `git -C <worktree> rev-parse HEAD` equals the base | 0 |
| worktree clean | `git -C <worktree> status --porcelain` empty | 0 |
| no pull request | `gh pr list --head phase/17-re-run-merge-and-clean-state --state open` returns 0 | 0 |

The Windows failure was a fault in plan 02's script, not in the diagnose prune: the manifest read failed under the Windows shell's path rewriting, so the three cases that read the release manifest answered `unknown` (exit 2).
