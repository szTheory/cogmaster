# Phase 11 deferred items

Out-of-scope findings recorded during execution; nothing here blocks a plan.

## Rehearsal cleanup reads a deleted tag back (found in plan 07, pull request run 43112269415)

`scripts/publish.sh` `rehearsal_delete` issues `DELETE git/refs/tags/<tag>` and then reads `git/ref/tags/<tag>`
once; on the first draft pull request run the read still answered 200, the cleanup job failed with "tag ... still
exists", and `gh api` a minute later found no `rehearsal-*` tag and no rehearsal release. The previous nine pull request runs that ran the
rehearsal all passed cleanup, so this is a read-after-delete race (one in ten), not a leak. `gh run rerun --failed`
turned the run green (cleanup and the required roll-up only). A fix would retry the existence read two or three
times before failing; it touches the release path, so it is left for a decision with the owner and was not made here.
