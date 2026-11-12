# Phase 11 deferred items

Out-of-scope findings recorded during execution; nothing here blocks a plan.

## Rehearsal cleanup reads a deleted tag back (found in plan 07, pull request run 43112269415)

`scripts/publish.sh` `rehearsal_delete` issues `DELETE git/refs/tags/<tag>` and then reads `git/ref/tags/<tag>`
once; on the first draft pull request run the read still answered 200, the cleanup job failed with "tag ... still
exists", and `gh api` a minute later found no `rehearsal-*` tag and no rehearsal release. The previous nine pull request runs that ran the
rehearsal all passed cleanup, so this is a read-after-delete race (one in ten), not a leak. `gh run rerun --failed`
turned the run green (cleanup and the required roll-up only). A fix would retry the existence read two or three
times before failing; it touches the release path, so it is left for a decision with the owner and was not made here.

Update (plan 09): the cleanup failed the same way on the next pull request run that ran the rehearsal (43117518796), so
the last two such runs both failed it (43112269415 and 43117518796) after nine that passed. Each time `gh api` found no
`rehearsal-*` tag or release afterwards, and `gh run rerun <id> --failed` (the cleanup job and the roll-up only) turned
the run green. The plan does not allow a change to the release path, so the retry stays a seed (SEED-014) and a push that
runs the rehearsal may need one failed-job rerun.

Update (review fix round 1): `rehearsal_delete` now reads each of the three things it deleted (the release by id, the
release list, the tag) up to six times five seconds apart through `gone`, and issues the delete again, with its error
shown, before it fails. `rehearsal-delete` is called only by the `cleanup` job of `ci.yml`, so the change touches a
rehearsal-tag-only path and not the release path; `policy.publish_delete` (hygiene lane, fake `gh` and `sleep` on
`PATH`) passes for a lag of two reads, which the old function failed, and for a tag that stays it fails after exactly
six reads.
