---
phase: 11-pal-and-japanese-fm
fixed_at: 2026-11-11T00:00:00Z
review_path: .planning/phases/11-pal-and-japanese-fm/11-REVIEW-RUNNER.md, .planning/phases/11-pal-and-japanese-fm/11-REVIEW-LEDGER.md
iteration: 1
findings_in_scope: 15
fixed: 15
skipped: 0
status: all_fixed
---

# Phase 11: Code Review Fix Report

**Fixed at:** 2026-11-11
**Source reviews:** `11-REVIEW-RUNNER.md` (runner and fixture half), `11-REVIEW-LEDGER.md` (ledger, library judge and CI half)
**Iteration:** 1 of at most 2

**Summary:**
- Findings in scope: 15 (runner WR-01, WR-02, IN-02, IN-03, IN-04, IN-06, IN-07; ledger WR-01 to WR-08)
- Fixed: 15, skipped: 0, disputed: 0 (each finding was checked against the code first; every one held)
- Out of scope, appended to `.planning/seeds/SEED-014-pal-fm-follow-ups.md` under "Phase 11 review leftovers" (append-only):
  the owner decision about CODEOWNERS and required review, the two fixture blind spots (`vdp_vtable` rotation
  invariance, `pause_nmi` reading V about 350 ticks after the NMI), runner IN-01 and IN-05, ledger IN-01 to IN-07, and two
  suggestions from the fix text (accept only `HTTP 404` as "gone"; cut the process spawns of `update.cmake`).
- No ROM, BIOS, library file or `.env.local` was opened. Every control uses self-authored fixtures or synthetic text.

## Fixed Issues

### Runner WR-01: the reset-script rejection rested on a false premise

**Files modified:** `runner/main.c`, `runner/input.c`, `runner/input.h`, `docs/runner.md`, `tests/runner/tests.cmake`
**Commit:** ce3e80b
**Applied fix:** `reset=1` in an `--input` script is the first console's Reset button on port `$DD` (`src/io.c`
`port_dd`); the runner never calls `cogmaster_reset`, so no time restarts. Removed `reset_seen` from
`cogmaster__input_validate` (input.c and input.h are back to their `origin/main` text), the refusal block in `run()`, and the
sentence in `docs/runner.md`, which now says the button restarts nothing. The reset block of the `expect_audio` case is one
test: a three-record script with `reset=1` on records 2 and 3, `psg_sweep` on `--machine sms1 --region ntsc-u`, must pass
`audio@3=<hash>` and the hash must be the one `--hash-audio` prints for the same run.
**Negative control:** with `runner/main.c`, `input.c`, `input.h` restored to the reviewed commit, `runner.expect.audio` fails
(`expected exit 0 and predicate kind=4 frame=3 address=0000 pass`, the old exit 2); with the fix it passes.

### Runner WR-02: `fm_sweep_jp` was the only new image whose digest nothing checked

**Files modified:** `tests/fixtures/tests.cmake`, `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt`
**Commit:** 6a7e9f8
**Applied fix:** `fixtures.fm_sweep_jp.pin` runs the existing `CASE=pin` preamble (the one `fixtures.bios_shape.pin.*` use) with
`-DNAME=fm_sweep_jp`. The other digest tests (`vdp_vtable_*`, `pause_nmi_*`) sit in the mailbox preamble and need no change.
Labels `fixtures,portable`, sorted into `ci.txt` and `wasm32.txt` after `fixtures.fm_sweep.sms-japan` (those are the only two
inventories that list fixture tests). Timeout 20 s floor, comment names the pin case's hosted time and says provisional, as the
surrounding fixture comments do.
**Negative control:** with the `fm_sweep_jp` line of `tests/built/expected.txt` changed to a wrong digest the test fails;
restored, it passes.

### Runner IN-02, IN-03, IN-04, IN-06

**Files modified:** `runner/main.c`, `runner/expect.c`, `runner/audio.h`, `docs/runner.md`, `tests/runner/tests.cmake`
**Commit:** fb42116
**Applied fix:** IN-02: the refusal reads "no such console was made for this machine, region and FM setting" (exit 77
unchanged). IN-03: "prints no `audio-` line" (the statement held only for `--hash-audio` alone). IN-04: `main.c` hands
`audio_hex` over exactly on frames that carry an audio case, so the `NULL` test and the `none` print are gone, with a one-line
comment. IN-06: one comment above `rate_num` and `rate_den`.
**Negative control:** IN-02 only (the others change no behaviour): `runner.usage.fm` now matches
`unsupported: no such console [^\n]*FM`; with the old text it fails, with the new text it passes.

### Runner IN-07: no timeout comment on the wrong-profile controls

**Files modified:** `tests/fixtures/tests.cmake`
**Commit:** f2d8dee
**Applied fix:** each control has its own `# Timeout:` line from the hosted per-test table of run 43117518796 (9 lanes, 1 run):
`fixtures.vdp_vtable.control.wrong_profile` 2 x 0.16 s under the 10 s floor, `fixtures.pause_nmi.control.wrong_profile`
2 x 0.45 s under its existing 20 s. No limit changed.
**Negative control:** none (comments).

### Ledger WR-01: `rehearsal_delete` read a deleted tag back once

**Files modified:** `scripts/publish.sh`, `tests/hygiene/tests.cmake`, `tests/inventory/hygiene.txt`,
`.planning/phases/11-pal-and-japanese-fm/deferred-items.md`
**Commit:** d22c7ee
**Applied fix:** `gone` reads up to six times with a 5 s pause (the idiom of the `verify` loop); the release by id, the release
list (`has_release`) and the tag each go through it, and before the final failure the delete is issued again with its error
shown. `rehearsal-delete` is called only by the `cleanup` job of `ci.yml`, not by `release.yml`, so this is a
rehearsal-tag-only path. `deferred-items.md` says so; SEED-014 records it as closed.
**Negative control:** `policy.publish_delete` (hygiene lane) puts a fake `gh` and a no-op `sleep` first on `PATH`. LAG=2 must exit
0 after 3 reads (old function: exit 1 after 1 read, "tag rehearsal-1-1 still exists"); LAG=99 must exit 1 naming the tag with
the read counter at exactly 6. Also run under dash and busybox sh in a container: same four results.

### Ledger WR-02: an allow line could not name one of two twin rows

**Files modified:** `scripts/compat-common.sh`, `tests/compat/ratchet.cmake`, `docs/conformance.md`
**Commit:** be63249
**Applied fix:** an optional `<region>/<tv>` token after the predicate narrows the match (`export/pal`); the line still has to
match exactly one passing base row. The ratchet records each row's tv for the comparison. `docs/conformance.md` documents the
token.
**Negative control:** `compat.ratchet` builds two twins (export ntsc, export pal) with the pal value flipped: a line with no
profile exits 2 (ambiguous), `export/pal` exits 0, `export/ntsc` exits 2 (the changed pal row is not excused), `japan/ntsc`
exits 2 (no such row). On the old script the `export/pal` case fails (exit 2, wanted 0). Same four results with mawk, gawk and
busybox awk in a container.

### Ledger WR-03: `docs/testing.md` claimed a review that does not exist

**Files modified:** `docs/testing.md`
**Commit:** 63126ec
**Applied fix:** the sentence now says a pull request runs its own `ci.yml`, that a `ci.yml` edit already forces the rehearsal,
and that the comparison guards against mistakes and regressions, not against an author who can edit `ci.yml`; the merge is the
owner's. No CODEOWNERS file and no ruleset change; the decision is a seed line.
**Negative control:** none (prose).

### Ledger WR-04: timeout comments overstated their sample

**Files modified:** `tests/compat/tests.cmake`, `tests/library/tests.cmake`, `tests/runner/tests.cmake`
**Commit:** e897644
**Applied fix:** I re-counted from the per-test times of the six successful runs of this branch
(`sh scripts/ci-durations.sh tests <run>`), counting only runs after the test's body last changed.
`compat.public_update`: 3 runs (43115877197 was killed at 30 s and gives no time), slowest 45.10 s, 91 s kept.
`compat.ratchet`: 4 runs since the parser last changed, slowest 48.74 s (run 43115877197), 98 s kept.
`compat.fixtures`: 4 runs, slowest 44.30 s (run 43119601937, not the 44.07 s the comment named), 89 s kept.
`library.synthetic`: 4 runs, slowest 54.22 s (run 43119601937, not 40.85 s), 120 s floor kept.
`runner.hashes_file` and `runner.expect.audio`: 6 runs each, slowest 53.07 s and 56.72 s, 107 s and 114 s kept.
No limit moved. I kept 2x: the recorded rule (`docs/testing.md`) is twice the slowest of the runs there are with N stated, and a
3x would depart from it, so N is stated honestly instead. The lane names were dropped from the rewritten comments where
`ci-durations.sh` does not report a lane. The spawn-cutting suggestion went to the seed.
**Negative control:** none (comments).

### Ledger WR-05: a digest, a RAM byte or a mailbox on a catalog title parsed

**Files modified:** `scripts/compat-common.sh`, `tests/compat/tests.cmake`
**Commit:** 56d3f16
**Applied fix:** one rule after the existing `audio` rule: a 40-hex id with any predicate but `boots` is refused in every mode
except the candidate half of `update`, which keeps its discard semantics (the existing `compat.public_update` rows for a
catalog id with `frame`, `ram` and `mailbox` still leave the baseline intact). The committed ledger is still a fixed point of
`normalize`.
**Negative control:** `compat.csv` takes a catalog-id row: with `boots@20` it must normalize (the control that the id alone is not
what is refused); with `frame@20=<digest>`, `ram@20:$C200=0A` and `mailbox@20` it must exit 2. On the old script it fails.

### Ledger WR-06: `built/` ids were accepted and judged by nothing

**Files modified:** `scripts/compat-common.sh`, `tests/compat/tests.cmake`, `tests/compat/update.cmake`
**Commit:** b4f346e
**Applied fix:** `grep` finds no row, test, doc or script that judges a `built/` id (`results`, `compat.fixtures`, stage 3 and
`library.cmake` look at `fixture/` only; no ledger row uses it), so it is dropped from the id grammar and from the `audio` rule.
`update.cmake` had one row that was discarded for being `built/`: that line is removed (discards 16 to 15) and replaced by a
check that a candidate with a `built/` id is refused (exit 2).
**Negative control:** `compat.csv` rejects `built/a` and `compat.public_update` expects exit 2 for the candidate; on the old
script both fail.

### Ledger WR-07: `md()` produced different pages on different awks

**Files modified:** `scripts/compat-common.sh`, `tests/compat/tests.cmake`
**Commit:** a262079
**Applied fix:** the four replacements are written `"\\&amp;"`, `"\\&lt;"`, `"\\&gt;"`, `"\\&#124;"`.
`docs/compatibility.md` is byte identical to a fresh generation (`cmp`), and a title and an evidence text holding `< > & |`
give the same page bytes on the macOS awk (20200816), mawk 1.3.4, gawk and busybox awk (alpine container): `A&lt;B&gt;&amp;&#124;C`.
**Negative control:** `compat.csv` generates the page for a title `A<B>&|C` and requires `| A&lt;B&gt;&amp;&#124;C |`. On the old
script it fails on the macOS awk (and on gawk and busybox awk it printed `A<lt;B>gt;&amp;|#124;C`); mawk read the old text correctly.

### Ledger WR-08: a non-verdict on an existing catalog row refused the whole merge

**Files modified:** `scripts/library.cmake`, `tests/library/tests.cmake`
**Commit:** 79a8577
**Applied fix:** the public fact of the first loop is written only when `result_status` is `pass` or `fail`, as the adopt loop
already does.
**Negative control:** `library.synthetic` seeds the stand-in ledger with a `pass` row for a catalog id and runs the judge with a
wrapper runner that answers the probe with that identity and fails every case (exit 1, an `infrastructure_error`). It must print
`merge: rows_added=0 ratchet=ok` and leave the row a `pass`. On the old script the run exits 1 (`merge: held (ratchet refused)`) and
the test fails. No owned image has a catalog identity, so the wrapper supplies it; no real title is involved.

## Verification

Run in the main checkout (`workflow.use_worktrees` is false), from the repository root.

| Command | Exit |
|---|---|
| `cmake --workflow --preset ci` | 0 (652 of 652; `ci-release` 5 of 5) |
| `cmake --workflow --preset hygiene` | 8 on the first run: only `hygiene.tree` failed, on a literal home directory path in the untracked ledger review file (rewritten to prose in that file); every other test passed (73 of 74) |
| `sh scripts/hygiene.sh --tree` after that rewrite | 0 |
| `ctest --test-dir build/hygiene -R hygiene.tree` after this report was written | 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | 0 (860 check lines, 9,402 src lines; limit 940) |
| gcc:13 container, `-std=c17 -Wall -Wextra -Wpedantic -Werror -Wtype-limits -Wconversion -fsyntax-only` over `runner/main.c`, `expect.c`, `args.c`, `audio.c`, `input.c` | 0 each |
| `clang-format` 21 `--dry-run -Werror` over `runner/main.c`, `expect.c`, `audio.h`, `input.c`, `input.h` | 0 |
| `sh -n` of `scripts/publish.sh` and `scripts/compat-common.sh` under dash and busybox sh (ubuntu 24.04 container) | 0 |
| the fake-`gh` scenario of `policy.publish_delete` under dash and busybox sh | LAG=2: exit 0, 3 reads; LAG=99: exit 1, 6 reads |
| the twin-row ratchet scenario and `normalize` of the ledger under mawk, gawk and busybox awk (containers) | rc 2, 0, 2 as expected; ledger is a fixed point |
| page for a title and an evidence text with `< > & |` under macOS awk, mawk, gawk, busybox awk | identical `&lt;`/`&gt;`/`&amp;`/`&#124;`; `docs/compatibility.md` unchanged |
| `git push origin phase/11-pal-and-japanese-fm` | 0 (ab08de1..eed3e34); no CI dispatched, PR left as it was |

Budgets after the round: counted check code 860 (was 859; ceiling 906, hard limit 940). Docs bytes: `docs/runner.md` 34,048
(was 34,054), `docs/testing.md` 32,057 (was 31,936), `docs/conformance.md` 32,320 (was 32,217); cap 38,912. The `quality.json`
floors were not touched and stay below the counts; the two new tests are `fixtures.fm_sweep_jp.pin` (ci and wasm32 inventories)
and `policy.publish_delete` (hygiene inventory), labels `fixtures,portable` and `hygiene,policy`, alphabetically placed.

Timeouts of the new tests: `fixtures.fm_sweep_jp.pin` 20 s floor and `policy.publish_delete` 30 s floor, both provisional (no
hosted time exists); the fixture and hygiene files already mark the like tests provisional. The next hosted run settles them.
The tests whose body this round changed (`runner.expect.audio`, `compat.ratchet`, `compat.csv`, `compat.public_update`,
`library.synthetic`) each gained a few cases; their limits are far above the local times and are unchanged.

Deviations:
- Several fixes share a file but each has its own commit; WR-06 and WR-07 touch the same line neighbourhood of `compat-common.sh`
  and stay separate.
- The ledger review file held a literal home directory path, which `hygiene.tree` (rightly) refuses; I rewrote that phrase in the
  untracked review file. The review files were not committed here.
- No CODEOWNERS or ruleset change (ledger WR-03), as instructed.

---

_Fixed: 2026-11-11_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
