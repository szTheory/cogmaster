---
phase: 05-tune-up
reviewed: 2026-11-10T00:00:00Z
depth: standard
files_reviewed: 31
files_reviewed_list:
  - AGENTS.md
  - .github/dependabot.yml
  - .github/workflows/ci.yml
  - .github/workflows/dependabot-automerge.yml
  - .github/workflows/digest.yml
  - .github/workflows/nightly.yml
  - .github/workflows/release.yml
  - README.md
  - quality.json
  - runner/png.c
  - scripts/ci-durations.sh
  - scripts/digest.sh
  - src/battery.c
  - src/cart.c
  - src/instance.c
  - src/synth.c
  - tests/cart/test_cart.c
  - tests/cmake/budgets.cmake
  - tests/cmake/ci_required.cmake
  - tests/cmake/quality.cmake
  - tests/cmake/readme.cmake
  - tests/cmake/repository.cmake
  - tests/cmake/repository.sh
  - tests/core/test_core.c
  - tests/fixtures/play.h
  - tests/hygiene/tests.cmake
  - tests/libretro/tests.cmake
  - tests/policy/budget_cases.cmake
  - tests/policy/tests.cmake
  - tests/retroarch/compare.c
  - tests/runner/tests.cmake
findings:
  critical: 0
  warning: 5
  info: 7
  total: 12
  necessity: 0
status: issues_found
---

# Phase 5: Code Review Report

**Reviewed:** 2026-11-10T00:00:00Z
**Depth:** standard
**Files Reviewed:** 31
**Status:** issues_found

## Summary

Reviewed the phase 5 diff against `f69ab03` for the listed files, with the `.github` workflows, the digest and
durations scripts, the quality ratchet, the cart size rule and the merged BMP comparator read in full. I ran
`scripts/digest.sh`, `scripts/ci-durations.sh summarise`, `tests/cmake/ci_required.cmake` and
`tests/cmake/quality.cmake` locally. The necessity rubric was read; no necessity finding applies.

No critical issue. What holds up:
- The Dependabot auto-merge workflow is safe against title injection. It uses `pull_request`, never checks out code,
  and reads the title only through a file and a `sed` that emits digits.
- The `CI required` roll-up and its actor clause are consistent with the `rehearsal` job guard.
- The cart refusal is correct at the boundaries: 1 MiB + 512 with a header is accepted, and 1 MiB + 1 to 1 MiB + 511
  with no header are refused. `CART_MAX` is still needed for larger images.
- `compare.c` bounds checks are sound. The header is read only after `end >= 70`, `*w <= n` and `rows <= n` hold,
  and `offset + stride * rows <= n` bounds every row read and the allocation.
- The timeout arithmetic in `ci-durations.sh` (`ceil(2 s / 60)`), `repository.sh` (`m * 60 >= 2 t`) and the digest
  margin test (`2 s > limit * 60`) agrees across all three.

The main concerns are a weakened release safety net for Dependabot pull requests, a probable Dependabot-versus-title-check
stall, a stale user-facing runner message after the cart change, and untrusted text flowing into the digest issue.

Per AGENTS.md rule 7, findings that concern test or tooling code (WR-05 and IN-01 to IN-07) are seeds, not
requirements.

## Warnings

### WR-01: Dependabot auto-merges release-pipeline action pins with the rehearsal skipped

**File:** `.github/workflows/ci.yml:634` and `.github/workflows/ci.yml:767` (with
`.github/workflows/dependabot-automerge.yml:37-55`)
**Issue:** `classify-changes.sh` sets `release_inputs=true` when `release.yml` or `ci.yml` changes, because the
rehearsal exists to prove the publish steps before they run on `main`. Dependabot's weekly group bumps `uses:` pins
in those two files: `actions/attest`, `download-artifact`, `upload-artifact`, `create-github-app-token` and
`release-please-action`. For those pull requests the rehearsal is now skipped by author, `CI required` accepts the
skip, and the auto-merge workflow enables the squash merge. A regression in a minor or patch of `actions/attest` or
`download-artifact` therefore reaches `main` unrehearsed. It first shows up on the release run, which AGENTS.md says
every phase ends with and which must not be held. The stated reason (the read-only Dependabot token cannot run the
rehearsal) is true, but it argues for not auto-merging those pins, not for skipping the check.
**Fix:** Do not auto-merge a Dependabot pull request that touches the release path. Either:
- add an `ignore:` or separate group in `dependabot.yml` for the release-path actions, so they stay open for the
  agent to rehearse by re-pushing from a branch the App token can use; or
- have the auto-merge job read the changed files and refuse when `ci.yml` or `release.yml` lines using those actions
  changed:
```sh
gh pr diff "$PR" --repo "$GITHUB_REPOSITORY" --name-only | grep -Eq '^\.github/workflows/(release|ci)\.yml$' &&
  { echo "dependabot-automerge: release inputs changed; not merging"; exit 0; }
```
Note that `ci.yml` is also the file holding the build pins, so the second option needs a finer test than path alone.
The first is simpler.

### WR-02: The title job's body scan will likely fail Dependabot pull requests, so they never auto-merge (not yet reproduced)

**File:** `scripts/check-title.sh:20` (run from `.github/workflows/ci.yml` job `title`), affecting
`.github/workflows/dependabot-automerge.yml`
**Issue:** `check-title.sh` fails any pull request whose body matches `[skip ci]`, `[ci skip]` and the like, because
the squash commit carries the body and the token would skip CI on `main`. Dependabot bodies embed upstream release
notes and commit lists. Actions published with semantic-release, which `create-github-app-token` is, conventionally
have commits ending in `[skip ci]`. Such a pull request goes red on `title`, `CI required` fails, and the auto-merge
stays pending until the digest reports a red PR. No Dependabot pull request exists yet, so this is an expectation from
upstream conventions, not an observed failure. The check itself is correct for the squash: a skip token must not reach
`main`.
**Fix:** Keep the check and remove the token from the commit instead. In `dependabot-automerge.yml`, pass an explicit
body so the squash message cannot carry upstream text:
```sh
gh pr merge --auto --squash --body "" "$PR" --repo "$GITHUB_REPOSITORY"
```
Then in `ci.yml` job `title`, skip only the body scan when `github.event.pull_request.user.login` is
`dependabot[bot]` (keep the title checks). Confirm `gh pr merge --auto --body` is honoured by the first real
Dependabot pull request.

### WR-03: Runner's size refusal message now states a range that does not load

**File:** `runner/main.c:249` (consequence of `src/cart.c:169-171`)
**Issue:** The message reads `an image of N bytes; 1024 to 1049088 load`. After the new body rule, sizes 1048577
through 1049087 without a copier header, and 1049089 and up, are refused, so the range wording tells a user that a
refused size should load. This is the only user-facing text for the new refusal, and `docs/runner.md` and
`docs/embedding.md` were updated to the body rule but this string was not.
**Fix:**
```c
fprintf(stderr, "cogmaster-run: unsupported: an image of %lu bytes; the body after any 512-byte copier header "
                "must be 1024 bytes to 1 MiB\n", (unsigned long)image_size);
```

### WR-04: Untrusted pull request titles are written verbatim into a public issue body

**File:** `scripts/digest.sh:48` (written by `.github/workflows/digest.yml:96-109`)
**Issue:** `$stuck` interpolates `.title` of every open pull request that is red or idle, including fork pull requests,
straight into the Markdown issue body. A title such as `fix: @somebody ...` or `fix: [x](http://...)` makes the Actions
bot notify arbitrary users and renders attacker-chosen links in an issue the owner reads weekly. It cannot reach the
`attention:` line, because every item is prefixed `- `. It does notify and render, so it is injection into Markdown,
not into the script. The fork `title` check does not constrain the characters after the type prefix.
**Fix:** Neutralise the title when formatting, for example in the jq program:
```jq
def plain: gsub("[@`<>\\[\\]]"; "") ;   # no mentions, links or HTML
... \(.title | plain)
```
or print `#\(.number)` only and let GitHub resolve the title.

### WR-05: The size ratchet's judge runs from the candidate tree, not the trusted base copy (tooling, seed)

**File:** `.github/workflows/ci.yml:146-147` and `.github/workflows/ci.yml:181`
**Issue:** The workflow preserves a base copy of `quality.json` but runs `tests/cmake/quality.cmake` from the head
checkout. The compatibility ratchet two lines above copies its judge from the base ("Preserve baseline judge and its
complete helper closure") for exactly this reason. A pull request that raises a ceiling can edit `quality.cmake`, or
the `policy.quality*` tests, in the same change and pass. The stated fault ("loosens with no recorded reason") is
met for accidental drift but not for a change that also touches the judge. The base `quality.cmake` is also absent when
the pull request introduces it, which the existing compat step already handles.
**Fix:** In the baseline step, `cp tests/cmake/quality.cmake "$trusted/"` when it exists, and call
`cmake ... -P "$trusted/quality.cmake"` for the ratchet run when that copy exists, falling back to the head copy.

## Info

### IN-01: A null or empty ceiling or floor silently disables the gate (tooling, seed)

**File:** `tests/cmake/quality.cmake:66-82` and `tests/cmake/quality.cmake:108-111`
**Issue:** Verified with a fixture: `{"ceilings": {"size.x": null}}` with a measured 999999 prints
`ceiling=` and exits 0. `string(JSON GET)` returns an empty string for null, `n GREATER ""` is false, and the ratchet's
`now GREATER was` is false too, so neither the limit nor the reason requirement fires. The limit is hand-edited JSON.
**Fix:** After each `GET`, require `ceiling MATCHES "^[0-9]+$"` (and the same for `floor`, `was`, `now`), else `FATAL_ERROR`
naming the key.

### IN-02: Digest prints "3 runs,10.8 job minutes" with no space after the comma

**File:** `scripts/digest.sh:57`
**Issue:** `...run\(...),\([$jobs[] ...` joins the run count and the job minutes without a space. It shows in the
job summary and issue body (`- CI: 3 runs,10.8 job minutes`). `digest.format` greps other lines, so the test does not
catch it.
**Fix:** Change `),\(` to `), \(` in that line.

### IN-03: Docs cap is exclusive while the same value is an inclusive ceiling; other caps are still hard-coded (tooling, seed)

**File:** `tests/cmake/budgets.cmake:12-18`
**Issue:** `budgets.cmake` fails when `size >= cap` (`NOT size LESS`), while `quality.cmake` treats the same
`docs.max_bytes` as inclusive (`n GREATER ceiling`). The header comment says quality.json is "the one place ceilings
and floors are kept", but `README.md=12288` and `include/cogmaster.h=24576` remain literals in this file, outside the
ratchet.
**Fix:** Move both caps into `quality.json` as ceilings, and use one comparison in both places.

### IN-04: `compare.c` relies on `play.h` for `<stdint.h>`, and the flip control can silently do nothing (tooling, seed)

**File:** `tests/retroarch/compare.c:1-4` and `tests/retroarch/compare.c:67-68`
**Issue:** The merge dropped `#include <stdint.h>`, so `uint8_t` and `uint32_t` now come transitively through the
oracle header. Separately, `--flip` flips a pixel only when `w[1] > 7 && h[1] > 3`; for a smaller image the negative
control changes nothing and the program reports equal, exiting 0, so a control step that expects failure fails for the
wrong reason.
**Fix:** Re-add `#include <stdint.h>`; return 2 with a message when `--flip` is given and the image is too small.

### IN-05: The digest and dependabot repository checks are regex tripwires with gaps (tooling, seed)

**File:** `tests/cmake/repository.sh:34-42` and `tests/cmake/repository.sh:43-49`
**Issue:** The `digest` check recognises only a bare `permissions:` followed by an indented block. An inline
`permissions: write-all` (top level or job) matches neither the `e=="permissions:"` branch nor any rule, so it passes.
The `dependabot` check never positively requires `on: pull_request`; it only forbids two named triggers, so
`on: [issue_comment]` with the App token also passes.
**Fix:** Add a rule for `^permissions: (write-all|read-all)` and one requiring `on: pull_request` in the automerge file.

### IN-06: Fragile or unbounded reads in the digest

**File:** `.github/workflows/digest.yml:73-74` and `scripts/digest.sh:20-27`
**Issue:**
- `gh pr list` and `gh issue list` use the default limit of 30, so a 31st open pull request or nightly issue is
  silently omitted.
- The `timeouts` awk recognises a job key only when the line is exactly `  key:` with nothing after it. A job written
  `  build:  # note` would not start a new job, and its `timeout-minutes` would be attributed to the previous job's key.
  No workflow does this today.
**Fix:** Add `--limit 200` to both `gh` calls; match `/^  [A-Za-z0-9_-]+:[ \t]*(#.*)?$/` in the awk.

### IN-07: Auto-merge treats every 0.x minor as safe and does not bind to the head it inspected

**File:** `.github/workflows/dependabot-automerge.yml:47-55`
**Issue:** "Same major" means `0.9.0 to 0.10.0` merges, although 0.x minors are breaking by semver convention.
`gh pr merge --auto` is also not given `--match-head-commit`, so a commit pushed to the branch after the title check
is auto-merged on CI alone. Both are small next to WR-01. The first step also checks only the client ID, not the
private key, so a missing Dependabot secret fails later inside the token action with a less direct message.
**Fix:** Require `$1 != 0` for the single-update case, and add `--match-head-commit "${{ github.event.pull_request.head.sha }}"`
through an env var.

---

_Reviewed: 2026-11-10T00:00:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
