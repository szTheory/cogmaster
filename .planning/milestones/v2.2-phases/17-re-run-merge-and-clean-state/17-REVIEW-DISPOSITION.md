---
phase: 17-re-run-merge-and-clean-state
review: 17-REVIEW.md
recorded: 2026-11-15
---

# Code review disposition

Every finding concerns tooling or test code (scripts, test files, CI comments), so rule 7 makes each a seed, not a requirement and not a fix round. Seeds are appended to `.planning/seeds/SEED-015-whole-list-follow-ups.md` under "Phase 17 code review".

| ID | Severity | Disposition | Note |
|----|----------|-------------|------|
| WR-01 | Warning | seed | `perf` job timeout comment says 2 x 106 s while the value stays 6 minutes |
| WR-02 | Warning | seed | `release-pr` row ignores `incomplete_results` from the search endpoint |
| WR-03 | Warning | seed | `--path-format=absolute` needs git 2.31 or newer; older git aborts `library.sh` under `set -eu` even with the variable exported |
| WR-04 | Warning | seed | linked-worktree fixture commit lacks `-c commit.gpgsign=false` |
| WR-05 | Warning | seed | `milestone-close.sh` is classed `operation`; counted as a check it would exceed the repository-check budget (owner judgment) |
| IN-01 | Info | seed | three off-paths of the close check have no control |
| IN-02 | Info | seed | Windows path-conversion guard covers only the `git show` call |
| IN-03 | Info | seed | `hygiene.sh` and `firmware-window.sh` still read `./.env.local` only |
| IN-04 | Info | seed | 60 s limit on `policy.milestone_close` is tight for a slow Windows host |
