---
phase: 11-pal-and-japanese-fm
reviewed: 2026-11-11T23:30:00Z
depth: standard
files_reviewed: 21
files_reviewed_list:
  - scripts/compat-common.sh
  - scripts/compat-update.sh
  - scripts/compat-ratchet.sh
  - scripts/library.sh
  - scripts/library.cmake
  - scripts/ci-durations.sh
  - scripts/hygiene.sh
  - scripts/publish.sh
  - .github/workflows/ci.yml
  - tests/compat/ledger.csv
  - tests/compat/migration.cmake
  - tests/compat/ratchet.cmake
  - tests/compat/update.cmake
  - tests/compat/tests.cmake
  - tests/library/tests.cmake
  - tests/hygiene/tests.cmake
  - docs/compatibility.md
  - docs/conformance.md
  - docs/testing.md
  - README.md
  - ASSET_POLICY.md
findings:
  critical: 0
  warning: 8
  info: 7
  total: 15
status: issues_found
---

# Phase 11: Code Review Report (ledger, library judge, CI half)

**Reviewed:** 2026-11-11
**Depth:** standard, security-minded (tools run: mawk 1.3.4 and gawk 5.2.1 in a container, BWK awk on the Mac, shellcheck, actionlint, a local run of the bridge step, the local `compat.*` and `library.synthetic` tests: 11 of 11 pass, tracked files untouched)
**Scope:** the ledger, library-judge and CI half; the runner, fixtures and core half is a separate review.

## Summary

The bridge, the migration and the adopt path are sound. I could not make a hostile PR or a hostile candidate file
get an invalid or weakened ledger past them without help from a `ci.yml` edit. No critical finding. Eight warnings:
the largest are a flaky required gate (the rehearsal cleanup race, diagnosed below with a bounded fix and a
control), an allow-list that cannot express the twin rows the library judge now produces, and two claims
that are stronger than the code (the review the trust model leans on does not exist in the ruleset; the
commercial-digest rule is enforced only for `audio`).

Verified and clean: migration of the real v0.10.0 ledger (`normalize(v1)` is byte-equal to the v2 head, `normalize(v2)`
is a fixed point), CRLF accepted, BOM and trailing blank line refused (exit 2), identical output on BWK awk, mawk and gawk
for `normalize`, `cases`, `results`, `ratchet` and the page; the bridge step run end to end against a v1 base (migration line,
exit 0) and with a flipped status, a dropped row (exit 2); `shellcheck` and `actionlint` find nothing in the changed shell
(only runner labels that the installed actionlint does not know).

## Answers to the review questions

### (1) The CI bridge

- **Trigger.** Exact literal pair, read from the trusted copy (base) and the head file; a quoted, CRLF, BOM or any other header
  misses it and falls to the base judge, which exits 2 on a v2 file. Fails closed. `.gitattributes` has `eol=lf`.
- **What the trusted base judge verifies versus the head judge.** After the merge nothing changes (branch dead, base judge
  runs). In the migration PR the head judge reads both formats and applies the same ratchet; I confirmed it accepts the real
  pair and refuses flipped status, dropped row, a pal row turned ntsc, an `ntsc-j` row migrated with `fm=off`, a changed value.
- **Who judges the head judge's own change in the migration PR.** Nobody but the head tests and a human. `compat.trusted_judge`
  itself shows a candidate judge that exits 0 is accepted when it is the judge. This is the same trust as the existing
  first-introduction branch. A PR that changes `compat-common.sh` and the ledger together can make the head judge accept
  anything **only in a PR whose merge commit's ci.yml contains the bridge**. The bridge is in no ci.yml on main, so an unrelated
  hostile PR cannot reach it; this PR can, and a PR runs its own ci.yml anyway. I agree no capability is added.
  The justification text is wrong in one place, see WR-03.
- **Stage 3.** It is not independent evidence: `results.csv` is the head ledger's own pass/fail fixture rows, written by
  `compat.fixtures` only after every such row ran against the runner (so its presence means the test passed; a missing file
  exits 2). It protects `fixture/` rows only: a `built/` id is accepted by the parser and judged by nothing (WR-06), and a new
  `pass` row for a catalog id is never evaluated in CI (inherent: no ROMs), so the ratchet is a no-regression gate, not a
  validity gate. That is the existing design, stated here so the trust model is not read as stronger.
- **Post-merge dead code.** Dead as designed once the base header is v2; a revert to v1 does not match the pair. The v1/v2 literals
  and the duplicated `base_rev` line stay until a later phase deletes them (IN-05).
- **Shell.** `set -e` default (`bash -e -o pipefail`) is not defeated: the judge call is the last command of each branch, so its
  status is the step's. `[ ] && [ "$(head ...)" ]` inside `if` cannot abort. No masking found.

### (2) Migration

No key collision after the split: `any` folds to `export,ntsc` exactly as it folded to `ntsc-u`; `ntsc-j` becomes `japan,ntsc`
with `fm=on` only for `sms`/`sms1`; legacy `pal` becomes `export,pal`; two v1 rows could not collide before and cannot now.
Omitted versus explicit default `fm` is one key (refused as duplicate, tested). A v1 `pal` row with `gg` was legal in v1 and now
exits 2 on the whole ledger (not present in the real ledger, so inert). The awk `fm` default equals the core default for every
machine the core has (`sms2`, `sms1`, `gg`; `sg` does not exist in `include/cogmaster.h`) and every region, and `cases` always passes `--fm` explicitly, so the default cannot
drift a run. Status ratchet across formats is by key and holds (tested in `ratchet.cmake`).

### (3) Adopt path in compat-update

Conditions (a) to (f) are all present in `compat-common.sh:55-64`. I tried a candidate that carries: `frame`/`ram`/`mailbox`/`audio`,
a device token, `boots@599`/`601`, `fm=on` on an export row, a mapper or machine mismatch, a `fixture/` and a `built/` id, a
backdated and a future date, a macOS and a Linux home directory path in title and evidence. Everything outside boots@600 default-cfg on a
whitelisted profile is dropped or exits 2; title and evidence are replaced from the catalog and the head revision; nothing from
a file name reaches `public-facts.csv` (the judge writes catalog id, catalog machine and mapper, fixed profile text). Only a
candidate-controlled **date** survives (IN-02). Dedupe by whole line is correct because the judge is deterministic for one
image; two differing lines for one key would exit 2 ("update refused"), which is fail-closed.

### (4) Portability

Same bytes from BWK awk, mawk 1.3.4 and gawk 5.2.1 for every mode on the real ledger. One awk-dependent function remains, see WR-07.
`wc -c`, `$(( $(wc ...) ))` and `date -u +%F` are fine with BSD padding. No gawk-only function.

### (5) ci.yml

YAML valid. Apt retry flags are placed correctly (`-o` before the subcommand). The be32 value (15 min) follows the mechanical rule but
rests on an outlier (IN-04). The 91 s / 114 s / 107 s limits are exactly `2 x` the slowest observed time and no more, i.e. at
the rule's floor, with a sample too small for two of them (WR-04).

### (6) Cleanup race: see WR-01.

## Warnings

### WR-01: `rehearsal_delete` reads a deleted tag back once and fails the required gate (read-after-delete)

**File:** `scripts/publish.sh:157-168` (reads at 160, 163, 165)
**Issue:** The function deletes the release by id, then the tag ref (both with errors discarded: `> /dev/null 2>&1 || true`),
and reads each back **once**; any success of the read is a hard failure. Run 43112269415 and 43117518796 both failed at
"tag ... still exists" and `gh api` then found nothing a minute later, with no second delete in between.

Root cause from the code: a single read-after-delete against a store that does not give read-your-writes. The same shape exists
at two more reads in the function: the release by id (line 160) and `release_id` (line 163, a *list* endpoint, which the file's own comment
at line 154 already says lags in the other direction). Because the discarded `DELETE` can also have failed (the tag is
protected while an immutable release's removal is still propagating, "the release goes before its tag" says the comment above the
function), a stale read and a refused delete look identical to the code. The evidence (the tag is gone a minute later without
a second delete) fits the lagging read; the fix below covers both.

Cost: the job is part of `CI required`, so a flake here keeps auto-merge from firing (rule 10), 2 of the last 2
runs that ran the rehearsal, 2 of 11 overall. Note also that `rehearsal-delete` is called only by the `cleanup` job of
`ci.yml` (`release.yml` calls prepare, upload, attested, verify), so the deferred item's reason that the change "touches the release
path" does not hold: it touches a rehearsal-tag-only code path (the function refuses any other tag at line 177).

**Smallest bounded fix** (same idiom as the `verify` wait loop at lines 109-114: six tries, a pause; worst case about 30 s of the
5-minute job limit; re-issuing the delete covers a refused first delete):
```sh
# A delete can read back as still there for a short while: read up to six times, five seconds apart.
gone() { i=0; while :; do "$@" || return 0; i=$((i + 1)); [ "$i" -lt 6 ] || return 1; sleep 5; done; }

rehearsal_delete() {
  if [ -n "${2:-}" ]; then
    gh api -X DELETE "repos/$REPO/releases/$2" > /dev/null 2>&1 || true
    gone gh api "repos/$REPO/releases/$2" > /dev/null 2>&1 || fail "release $2 still exists"
  fi
  gh release delete "$1" --repo "$REPO" --yes > /dev/null 2>&1 || true
  i=0; while [ -n "$(release_id "$1")" ]; do i=$((i + 1)); [ "$i" -lt 6 ] || fail "$1 still has a release"; sleep 5; done
  gh api -X DELETE "repos/$REPO/git/refs/tags/$1" > /dev/null 2>&1 || true
  gone gh api "repos/$REPO/git/ref/tags/$1" > /dev/null 2>&1 || { gh api -X DELETE "repos/$REPO/git/refs/tags/$1" || true; fail "tag $1 still exists"; }
  echo "publish: $1 and its release are gone"
}
```
(I ran the release and tag part of this against a fake `gh` that answers "exists" for the first N reads after a delete:
N=0, 2, 5 exit 0 after N+1 reads; N=6 and N=99 exit 1 after exactly 6 reads.) Also stop discarding the DELETE's stderr on the
last attempt, so the next failure names the HTTP status.

Caveat to keep: `gh api` failing for any reason (5xx, expired token) counts as "gone", as today. If that matters, accept only
an error text containing `HTTP 404` and retry otherwise.

**Control** (add beside `policy.publish_set` in `tests/hygiene/tests.cmake`, hygiene label, about 15 lines, no `src/` lines): a
`sh -c` body that puts a fake `gh` and a no-op `sleep` first on `PATH`, `GITHUB_REPOSITORY=o/r`, and runs
`sh scripts/publish.sh rehearsal-delete rehearsal-1-1 42` with the fake answering "exists" for the first `LAG` reads of `git/ref/tags/*`
(state in a counter file). Assert: `LAG=2` exits 0 (fails against today's code: one read, exit 1); `LAG=99` exits 1, prints
`tag rehearsal-1-1 still exists`, and the counter is exactly 6 (bounded, and a real leak still fails).

### WR-02: A twin pair of rows cannot be named in `allow.txt`; the library judge now creates twins

**File:** `scripts/compat-common.sh:80-81`
**Issue:** An allow-list line names an id and a predicate; it is accepted only if it matches **exactly one** passing base key
(`matches != 1` is `bad()`, which exits 2 for the whole comparison). The library judge now emits `export,ntsc` and `export,pal`
rows for one cart with an identical id, `fm=off`, and identical `cfg.../boots@600` text. I reproduced it: two such rows, one
flipping `pass` to `fail`, and an allow line `<id> boots@600 ... behaviour-rev=17`: `compat: invalid ledger record 6`, exit 2.
So after the milestone-end merge (two rows per export cart) a behaviour change that legitimately moves one PAL `boots` row
cannot be recorded, and a stale or mistaken line cannot be told from a missing one. Inert today (no twin in the ledger).
**Fix:** let the line carry the profile, e.g. an optional token `region/tv` or `@export,pal` before the reason words, compared with
`regions[key]` and the `tv` of the key; keep `matches == 1`. Add one `ratchet.cmake` case (two twins, a flipped one, an exception
naming the pal one: exit 0; naming neither: exit 2).

### WR-03: The trust model leans on a review the ruleset does not require

**File:** `docs/testing.md:57-59`; `.github/rulesets/main.json:19`
**Issue:** The new text says a `ci.yml` edit "already forces the rehearsal and review". The committed ruleset has
`required_approving_review_count: 0`, `require_code_owner_review: false`, no CODEOWNERS file, and the only required check is `CI required`,
a job defined in the same `ci.yml` the PR can edit. The rehearsal half is true (`release_inputs`); the review half is not. The
conclusion (no added power) survives, but the stated reason does not, and the protected comparison is advisory against
anyone who can merge, including its own bridge for the single PR that carries it.
**Fix:** reword to what is true ("a pull request runs its own ci.yml, so the comparison guards against mistakes and
regressions, not against an author who can edit ci.yml; the merge itself is the owner's"), or add the review: CODEOWNERS for
`.github/workflows/**`, `scripts/compat-*.sh`, `tests/compat/**` with `require_code_owner_review: true`.

### WR-04: Timeout comments overstate their sample; the limits sit exactly at the 2x floor

**File:** `tests/compat/tests.cmake:406` (91 s), `tests/library/tests.cmake:187` (120 s), `tests/runner/tests.cmake:1605` (107 s) and `:2067` (114 s), `tests/compat/tests.cmake:377` (98 s)
**Issue:** Every limit is `ceil(2 x slowest)` with no margin beyond the rule (91 vs 90.2, 98 vs 97.8, 107 vs 106.14, 114 vs 113.4). The comments say "slowest of 4 runs". For `compat.public_update` that cannot be true:
its body reached the size it has at plan 8 (`e441ed2`, 17:48) and the first hosted run with it (43115877197) killed it at 30.02 s and 30.01 s on
Windows ARM64 (commit 2104092); the only measurement of the final body is 43117518796 (one run, one lane). `library.synthetic`
(plan 8, 17:52) has the same two-run history. A single sample times two is a thin ceiling for the lane that is already 18x
slower than the Mac for this test (2.47 s here, 45.10 s hosted; the cost is process spawns: about 55 `execute_process(sh ...)` each running two `git rev-parse` plus awk).
**Fix:** state the true N in the comments ("1 run since the test last changed") or take 3 x for tests with N < 4; cut spawns
instead of raising limits (batch the single-row `update()` cases of `update.cmake` into the existing "all together" call where the result is the same; compat-update.sh runs `git rev-parse` twice per call).

### WR-05: The commercial-digest rule is enforced for `audio` only

**File:** `scripts/compat-common.sh:53`; `ASSET_POLICY.md:23-24`
**Issue:** The new policy sentence says ledger rows for commercial titles "carry `boots` predicates only". The parser refuses `audio` on a
40-hex id, but `frame@F=HASH`, `ram@...` and `mailbox` on a 40-hex id still normalize, still pass the ratchet (I added a `pass` row with
a `frame@14=<hash>` for a catalog id to the real ledger: `compat: protected comparison passed`) and nothing in `compat.csv`
or the policy tests refuses it. Only `compat-update` is closed. A hand-edited PR can commit such a digest, which is exactly what the sentence promises never happens.
**Fix:** one rule in the row check: `if(f[1] ~ /^[0-9a-f]+$/ && kind != "boots") bad()` after `predicate()` (and update `update.cmake`, whose hostile rows
then exit 2 instead of being discarded, or apply the rule only in `normalize`/`ratchet` mode to keep the discard semantics of update); add one `compat.csv` reject case.

### WR-06: `built/` ids are accepted but judged by nothing

**File:** `scripts/compat-common.sh:51,53,88`; `tests/compat/tests.cmake:58`
**Issue:** The parser, and now the `audio` rule, accept `built/<name>`. `results` mode, `compat.fixtures` (`^fixture/`) and the stage-3 check (`ids[key] ~ /^fixture\//`)
look at `fixture/` only, and `library.cmake:38` maps an image to `fixture/` only. A `built/x ... pass ... audio@300=<anything>` row therefore has
no fresh evaluation in CI and cannot fail. Every `built/` row is an unevaluated `pass`. Pre-existing namespace, extended by this phase.
**Fix:** either drop `built/` from the id regex (no row uses it) or make `results`, `compat.fixtures` and stage 3 treat `(fixture|built)/` alike.

### WR-07: `md()` produces different pages on different awks

**File:** `scripts/compat-common.sh:17`
**Issue:** `gsub(/</,"\&lt;",s)`: mawk keeps the backslash escape and prints `&lt;`; BWK awk (the Mac's 20200816) and gawk read `"\&lt;"` as `&lt;` whose `&` then means "the matched text", giving `<lt;`
(`|` gives `|#124;`, `>` gives `>gt;`). I fed a title `A<B>&|C`: mawk `A&lt;B&gt;&amp;&#124;C`, BWK and gawk `A<lt;B>gt;&amp;|#124;C`. The page is generated on the owner's Mac and compared byte for byte on Linux (mawk) and Windows (git-bash gawk) by `policy.compat_doc_fresh`. Latent today: none of the 1,000+ catalog titles has `<`, `>` or `|` (39 have `&`, which works by coincidence in all three), but the milestone-end merge copies catalog titles into the ledger and the evidence column is `md()`-escaped too.
**Fix:** write the replacement with a doubled backslash, `"\\&amp;"`, which every awk reads as a literal `&`; add one `compat.csv` row with `<`, `>`, `|` and `&` in the title.

### WR-08: One transient non-verdict on an existing catalog row refuses the whole merge

**File:** `scripts/library.cmake:73-83`
**Issue:** For a catalog id already in the ledger, the judge appends a fact for every outcome, including `infrastructure_error` (any exit other than 0, 3, 77: a 30 s timeout on a loaded host, a missing runner) and `unsupported`. The adopt loop below it appends only `pass`/`fail`. `update` accepts the new status for an existing key and the ratchet then
refuses `pass` to `infrastructure_error` (and `fail` to anything but `pass`). Reproduced: ledger with one `pass` catalog row, a fact for it with `infrastructure_error`: update exit 0, ratchet exit 2, so `library.sh run` prints `merge: held (ratchet refused)`, exits 1, and every other row adopted in that run is thrown away.
**Fix:** write a public fact only when `result_status` is `pass` or `fail` (the same rule as line 166), so a non-verdict leaves the row as it was.

## Info

### IN-01: `library.sh run` with no ROM directory aborts instead of printing "skipped: not configured"

**File:** `scripts/library.sh:44`
**Issue:** With `COGMASTER_ROM_DIR` unset: `COGMASTER_ROM_DIR: unbound variable`, exit 1, no summary (I ran it in a scratch repository). With it set but empty (a `.env.local` that names only the BIOS folder: the sed prints nothing and the variable is exported empty) the glob becomes `/*`, scanning the filesystem root for `*.sms|gg|bin|rom` files. Harmless in practice, but the summary claims a no-input section prints `skipped`.
**Fix:** `[ -d "${COGMASTER_ROM_DIR-}" ] || { printf 'library-judge 1\npal: skipped: not configured\nfm: skipped: not configured\n'; exit 0; }` before the loop.

### IN-02: A candidate's date is kept, any valid day, including the future

**File:** `scripts/compat-common.sh:64`
**Issue:** `day()` checks the calendar only; `2999-12-31` from a candidate passes into the output row (reproduced). Evidence is forced to `core=<rev>`, the date is not forced to today. Cosmetic for the judge's own file (always today), but the update path is described as public-facing.
**Fix:** `if(day(f[10])>day(ENVIRON["COMPAT_TODAY"])) bad()` with the date passed in, or take today's date from the caller.

### IN-03: `library.sh` applies the ledger and the page with two separate copies

**File:** `scripts/library.sh:70-71`
**Issue:** `cp ledger` then `cp page`; if the second fails (disk full, a read-only page) the ledger is updated and `policy.compat_doc_fresh` is red. Also each run rewrites evidence and date of every existing catalog row it re-judges, so a clean no-change run still dirties the tree.
**Fix:** copy the page to a temp in the same directory and `mv` both after both writes succeeded; skip the facts of an unchanged status.

### IN-04: be32 timeout of 15 minutes is twice an apt stall that the same change tries to remove

**File:** `.github/workflows/ci.yml:310`
**Issue:** The rule is applied as written (2 x 445 s = 890 s), but the 445 s is the DNS stall of run 43062734904 against a typical 206 s; `Acquire::Retries=3` retries immediately and so may not shorten a stall of minutes, and the `update` step can also exit 0 on a failed index fetch. A real hang now costs 15 minutes of runner time instead of 10.
**Fix:** keep the job limit, add `timeout-minutes: 3` on the apt step (a stall then fails fast and `gh run rerun --failed` is cheap), or loop the apt calls with a pause.

### IN-05: The migration branch leaves a fourth and fifth copy of the header

**File:** `.github/workflows/ci.yml:198-200`; `scripts/library.sh:31`; `scripts/compat-common.sh:35`
**Issue:** The v2 header literal now lives in `compat-common.sh`, `library.sh`, `ci.yml` (twice, v1 and v2), the tests. A future format 3 changes all of them; `library.sh` could print the header from `normalize` of an empty ledger or read it with `head -n 1` of the tracked ledger. The `base_rev` line is duplicated between the two branches.
**Fix:** hoist `base_rev` above the `if` and delete the bridge in the first plan of the next phase (dead after the merge, as the comment says).

### IN-06: Controls that do not cover what their comment promises

**File:** `tests/hygiene/tests.cmake` (`hygiene.tree.header_offset`, `hygiene.analyzer`)
**Issue:** `hygiene.tree.header_offset` plants the marker at 32,752 only; the loop reads three offsets (32,752, 16,368, 8,176), so a read-once regression that dropped the other two passes. `hygiene.analyzer` now relies on `xargs` returning non-zero for a failing batch (123 on GNU, 1 on BSD); the planted-defect proof was a one-time throwaway, so a later edit that turns the pipeline into something that swallows the status (a trailing `|| true`, `xargs -r` plus a glob miss) has no control.
**Fix:** loop the planted file over the three offsets (the helper is a three-line `dd`); optionally a `clang-tidy` stand-in script that exits 1 passed as `$2` and `expect_fail.cmake` on the xargs line.

### IN-07: Tests assert `fm=off` for a `sg` machine row that the core would default to `on`

**File:** `tests/compat/migration.cmake:152`; `scripts/compat-common.sh:23`
**Issue:** The awk default is "on iff japan and machine is sms/sms1"; the core's is "japan and not gg". They agree on every machine the core has (the SG-1000 is not one), so no behaviour drifts, but the migration test and the comment "mirrors instance.c" claim an equivalence for a machine neither can run. Not a defect; remove the `sg` expectation or say it is inert.

---

_Reviewed: 2026-11-11_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
