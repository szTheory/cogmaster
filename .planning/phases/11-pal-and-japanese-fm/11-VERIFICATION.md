---
phase: 11-pal-and-japanese-fm
verified: 2026-11-12T00:30:00Z
status: passed
score: 4/4 must-haves verified
covered_files:
  - ".github/workflows/ci.yml"
  - ".planning/phases/11-pal-and-japanese-fm/11-01-PLAN.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-01-SUMMARY.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-02-PLAN.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-02-SUMMARY.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-03-PLAN.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-03-SUMMARY.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-04-PLAN.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-04-SUMMARY.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-05-PLAN.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-05-SUMMARY.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-06-PLAN.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-06-SUMMARY.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-07-PLAN.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-07-SUMMARY.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-08-PLAN.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-08-SUMMARY.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-09-PLAN.md"
  - ".planning/phases/11-pal-and-japanese-fm/11-09-SUMMARY.md"
  - "ASSET_POLICY.md"
  - "PROVENANCE.md"
  - "README.md"
  - "docs/audio.md"
  - "docs/compatibility.md"
  - "docs/conformance.md"
  - "docs/controllers.md"
  - "docs/maintaining.md"
  - "docs/runner.md"
  - "docs/testing.md"
  - "quality.json"
  - "runner/args.c"
  - "runner/args.h"
  - "runner/audio.c"
  - "runner/audio.h"
  - "runner/expect.c"
  - "runner/expect.h"
  - "runner/main.c"
  - "scripts/ci-durations.sh"
  - "scripts/compat-common.sh"
  - "scripts/hygiene.sh"
  - "scripts/library.cmake"
  - "scripts/library.sh"
  - "scripts/publish.sh"
  - "tests/audio/pcm.txt"
  - "tests/audio/transitions.txt"
  - "tests/built/expected.txt"
  - "tests/cmake/hashes.cmake"
  - "tests/compat/ledger.csv"
  - "tests/compat/migration.cmake"
  - "tests/compat/ratchet.cmake"
  - "tests/compat/tests.cmake"
  - "tests/compat/update.cmake"
  - "tests/diag/expected.csv"
  - "tests/fixtures/tests.cmake"
  - "tests/hygiene/tests.cmake"
  - "tests/inventory/ci.txt"
  - "tests/inventory/hygiene.txt"
  - "tests/inventory/wasm32.txt"
  - "tests/library/tests.cmake"
  - "tests/policy/repository-checks.txt"
  - "tests/roms/manifest.txt"
  - "tests/roms/src/header.inc"
  - "tests/roms/src/pause_nmi.asm"
  - "tests/roms/src/pause_nmi.link"
  - "tests/roms/src/vdp_vtable.asm"
  - "tests/roms/src/vdp_vtable.link"
  - "tests/runner/expect.cmake"
  - "tests/runner/tests.cmake"
covered_digest: "v4:sha256:c615c1bc9d0a755b32ab7fffcfc5aa3de4b03c6d775fd9465e5882584d8a372f"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 11: PAL and Japanese FM Verification Report

**Phase Goal:** PAL timing is proven identical on every lane, FM is judged by its sound on owned fixtures, and the library judge can add PAL and Japanese FM rows for catalog-identified carts under the right profile.
**Verified:** 2026-11-12
**Status:** passed
**Re-verification:** No, initial verification
**Branch / HEAD:** `phase/11-pal-and-japanese-fm` at 7ae2085 (draft PR #31 stays a draft)

Human verification items: none for phase completion. Every check below is a test, a command I ran, a negative control I
mutated, or a hosted run. One item is a documented owner hand-off and is not a gap (see "Carried hand-off").

The legal rule held: I read no ROM, no BIOS dump, no owner library and no `.env.local`. Every image used here is one
of the repository's own fixtures built from MIT source.

## Goal Achievement

### Observable Truths (ROADMAP Phase 11 criteria, criterion 4 as amended by the REG-04 amendment)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | A ledger migration splits `tv` from `region` with a format note, and every existing row keeps its status through it (REG-01) | VERIFIED | `scripts/compat-common.sh` reads format 1 (`ntsc-u`/`ntsc-j`/`pal`/`any`) and writes format 2 (`region` export or japan, `tv` ntsc or pal, `fm` in the run prefix). I ran `git show origin/main:tests/compat/ledger.csv` through `sh scripts/compat-common.sh normalize`: exit 0, 5 rows. By whole-row string comparison all 5 normalized rows appear byte-for-byte in the live `tests/compat/ledger.csv` (5 of 5 `grep -qxF`); the status multiset of the five is 4 `pass` and 1 `unknown` on both sides (the live file adds five more rows, all fixtures at `pass`). Fixed point: `normalize` of the live ledger equals the file (`cmp` exit 0) and normalizing the normalized output again is equal (exit 0). Format note: `docs/conformance.md` lines 66-73 ("Format 2 replaced the single region column ... A migrated row keeps its status, evidence and date and gains explicit values", with the PAL-row caveat); README carries the user lines (`--region pal`, **Region and timing (reload content)**, **FM sound unit (reload content)**, `--fm on|off`). Tests `compat.migration`, `compat.csv`, `compat.device_key`, `compat.ratchet` pass. Mutation: the normalize default `fm=(region=="japan" ...)` replaced by `fm="off"`: `compat.migration`, `compat.ratchet`, `compat.fixtures` fail (3 of 11); restored. |
| 2 | PAL fixtures for the V-counter tables, Pause at line 311 and exact samples per frame give equal hashes on all eight lanes (REG-02) | VERIFIED | Fixtures built from MIT source: `vdp_vtable_{192,224,240}`, `pause_nmi_{192,224,240}`, plus `fm_sweep` under PAL with FM on. `tests/cmake/hashes.cmake` writes, per lane, the digest of the printed mailbox records of the V and Pause images (PAL at all three heights, NTSC at 192 and 224), the PAL Pause device frame (`playing pause_nmi_*.sms sms pal device frame 40`), and the `audio-transitions` and `audio-pcm` lines of `fm_sweep.sms sms pal fm-on 300`; the same bytes are first judged against literal values in `tests/diag/expected.csv` (PAL count 0x139, Pause first NMI after 346 / 252 / 205 iterations by height, `v_at_first_nmi` 0xFF), so equal hashes cannot be equal and wrong. Hosted: D1 dispatch run 43111533403 (head 1c720da), `hash-equality` success, log line `hashes: 8 identical files`; I downloaded the eight `hashes-*` artifacts and `shasum -a 256` gives one value for all eight (`6c7ed14c...39a2f`, 17,502 bytes each: linux-x64, linux-arm64, macos-x64, macos-arm64, windows-x64, windows-arm64, be32, wasm32); 19 `vdp_vtable`/`pause_nmi` rows in that file. My local macOS arm64 `build/ci/hashes.txt` from the current head differs from the D1 file only by the added `fixture fm_sweep_jp.sms` line and a stale local `boot_to_play_gg.gg` fixture row from an October build directory, not by any PAL row. PR runs 43119601937 (b95e2ea) and 43121365762 (ab08de1) both show `hash-equality` success after the Japan fixture and the library judge landed; the fix-round head 7ae2085 run 43126677381 shows `hash-equality` success (see Hosted evidence). Exact samples: `runner/audio.c` now enforces `want = floor(f x num / den) - floor((f-1) x num / den)` with num/den 3,504,512/4,375 (NTSC) and 685,094,400/709,379 (PAL) and fails the run unless `io.written == want` (replacing the old 965-or-966 tolerance); the PAL 300-frame line reads `samples 289729`, which equals floor(300 x 685,094,400 / 709,379). Mutations: Pause line `src/vdp.c:151` `- 2u` to `- 3u`: `vdp.pause_nmi`, five `fixtures.pause_nmi.mailbox.*` and `runner.hashes_file` fail (7 of 28); PAL den 709379 to 709378 in the runner: `runner.audio_cadence`, `compat.fixtures`, `runner.hashes_file` fail (3 of 8); each restored. |
| 3 | The `audio@F=H` predicate passes on a matching transition-stream hash and fails on its negative control, and `fm` is part of the row identity (REG-03) | VERIFIED | `runner/expect.c` kind 4 compares the running SHA-256 of the transition stream (`cogmaster__audio_transitions_hex` finalises a copy so later frames still accumulate); `runner/main.c` drains once for `--hash-audio` or an audio case. `runner.expect.audio` (tests/runner/tests.cmake lines 1298 on) takes the pins from the frozen `tests/audio/*.txt` files and a run made in the test (no typed digest): two audio cases plus a picture case pass together, a last-nibble flip fails with exit 3 and the observed hash printed, a prefix hash judged at the wrong frame fails, a PCM hash judged as a transition hash fails, the same ROM with `--fm off` fails (6,939 changes against 86,278), the PAL stream fails under NTSC, eleven malformed forms exit 2. Ledger rows `audio@300=...` for `fixture/fm_sweep` (export off, japan on) and `fixture/psg_sweep` (export ntsc, export pal, japan on) are re-run by `compat.fixtures`. Identity: the key is `id|machine|region|tv|mapper|config|parameter` and `config` carries `fm=`; by hand, two copies of one `fm_sweep` row differing only in `fm=off` and `fm=on` normalize to two rows (3 lines with header) and an exact duplicate exits 2. Mutations: judge made pass-always in `runner/expect.c`: `runner.expect.audio` fails; the Japan transition pin in `tests/audio/transitions.txt` given one extra digit: `runner.expect.audio`, `runner.hash_audio.fm`, `runner.hashes_file` fail; one hex digit of a ledger audio hash changed: `compat.fixtures`, `policy.compat_doc_fresh` fail; each restored. |
| 4 | The library judge adds PAL and Japanese FM rows for catalog-identified carts through `compat-update.sh`, proven in CI by a synthetic library run through `library.sh run`, and the ratchet passes (REG-04, as amended) | VERIFIED | `scripts/library.cmake` looks each cart up in the catalog (runner identity SHA-1, else headerless, else file SHA-1), takes machine and mapper from the catalog and the region from the probe (never the file name), runs `boots@600` under the default configuration on the derived profiles and appends rows; `scripts/library.sh run` prints `library-judge 1`, `pal:`, `fm:` and `merge:` as counts only, writes a candidate ledger through `compat-update`, runs the ratchet and the page-cap test, and copies over tracked files only when both pass. `scripts/compat-common.sh` update stage adopts a new key only if the id is a catalog id, machine and mapper equal the catalog's, the predicate is exactly `boots@600`, status is pass or fail, and the profile is on the judge's whitelist (export ntsc/pal off, japan ntsc on, Game Gear rows), with the default `cfg`; anything else is discarded, and impossible forms exit 2. `library.synthetic` (5.2 s) runs it from `library.sh run` on owned images with a temporary catalog: log lines `pal: carts=1 ran_pal=1 boots_pass=1`, `fm: carts=1 boots_pass=0 boots_fail=1`, `merge: rows_added=4 ratchet=ok page_bytes=5718`, and `merge: held (page over the docs cap)` with both stand-in files byte-identical; the Game Gear image named `.sms` and the Japan image `.gg` still yield the catalog's profiles; planted path and address strings reach no output; `compat.public_update` (five adoptions, sixteen discards, four non-evaluated statuses, five impossible forms) passes. Mutations: adopt-path condition `parameter!="boots@600"` removed: `compat.public_update` fails; restored. Amendment consistency: see "REG-04 amendment consistency" below. |

**Score:** 4/4 truths verified, 0 present-but-behavior-unverified.

### REG-04 amendment consistency

The amendment says: REG-04 is the judge, the guarded merge and the narrow adopt path, proven in CI on synthetic
images; the owner's real own-library run is a batched hand-off (SEED-013, status `not_run`, `human_judgment: true`),
neither a gate nor user acceptance. I found it consistent:

- `.planning/REQUIREMENTS.md` REG-04 (checked, traceability row Complete) and `.planning/ROADMAP.md` Phase 11 goal,
  criterion 4 and the top line all read "can add ... proven in CI by a synthetic library run through `library.sh
  run`"; the superseded "A local library run judges the owner's PAL and Japanese FM carts" wording is gone from the
  tracked docs (`git grep` of the old phrases outside the plan and summary history returns nothing).
- ROADMAP Phase 12 criterion 5 carries "including the PAL and Japanese FM rows of the run recorded in the seed".
- `.planning/seeds/SEED-013-pal-fm-library-run.md` holds the exact request (`sh scripts/library.sh run`, paste the
  `library-judge` block), says it is "never a gate and never a user acceptance step", names what the agent reads
  (the block and the ledger and page diff only, never `.path`, `.probe` or `.error`), and why the real merge waits
  (the page cap). SEED-010 gains the batching sentence; SEED-012 names it as the source of its trigger.
- `docs/conformance.md` says how the judge adds rows (the adopt rule), consistent with the code.
- Rule 6: the owner verifies nothing by default; a run on the owner's hardware is the identity-bound exception and
  is recorded as a seed, not a requirement. Rule 8: no requirement names a fact that only a person can show.

One observation, not a gap: `.planning/PROJECT.md` line 50 keeps the milestone-level item "PAL and Japanese FM over
the ledger, and the ledger over the whole CC0 MAME lists" unchecked; it closes with phase 12, so it is correct as is.

### Required artifacts and key links

| Artifact / link | Status | Details |
|-----------------|--------|---------|
| `scripts/compat-common.sh` (format 1 read, format 2 write, `fm` in identity, adopt rule) | VERIFIED | substantive; used by `compat-update.sh`, `compat-ratchet`, `library.sh`, docs generation; mutation of fm default and of the adopt condition each turn tests red |
| `tests/compat/ledger.csv` (10 columns, 10 rows) | VERIFIED | the five migrated rows are whole-row equal to the normalized origin/main ledger; five new fixture audio rows; `policy.compat_doc_fresh` ties `docs/compatibility.md` to it |
| `tests/roms/src/{vdp_vtable,pause_nmi}.asm`, `header.inc` (Japan header code) | VERIFIED | built in CI, digests pinned in `tests/built/expected.txt` (additions only, 0 removed lines), none committed as bytes, three manifest comment lines |
| `tests/cmake/hashes.cmake` -> `ci.yml` `hash-equality` (COUNT=8) | WIRED | write mode run by `runner.hash_matrix` on every lane; compare requires exactly eight byte-identical files |
| `runner/expect.c` kind 4 -> `runner/main.c` -> `runner/audio.c` | WIRED | one drain per run, transitions hash copy finalised per audio frame |
| `scripts/library.sh` -> `library.cmake` -> `compat-update` -> ratchet -> page cap | WIRED | exercised end to end by `library.synthetic` through `sh scripts/library.sh run` |
| `tests/inventory/*.txt`, `quality.json` floors | WIRED | see runs table |

### Data-flow trace

The judged values are produced by the core under test: a PAL V-counter record or Pause record comes from the core's
VDP and bus through the runner's `--mailbox`, the audio hash from the chips' transition log, the ledger rows from
real runner invocations in `compat.fixtures` and `library.synthetic`. No static return stands in for any of them
(the mutations above change the core line, the runner constants or the ledger hash and the matching test goes red).

### Requirements coverage

| Requirement | Code | Tests | Commands I ran | Result |
|-------------|------|-------|----------------|--------|
| REG-01 | `scripts/compat-common.sh`, `tests/compat/ledger.csv`, `docs/conformance.md`, `README.md` | `compat.migration`, `compat.csv`, `compat.device_key`, `compat.ratchet`, `policy.compat_doc_fresh` | normalize of origin/main ledger and whole-row comparison; fixed-point `cmp`; fm-default mutation | 5 of 5 rows equal, 4 pass and 1 unknown kept; fixed point; mutation red (3 tests) |
| REG-02 | `tests/roms/src/vdp_vtable.asm`, `pause_nmi.asm`, `tests/cmake/hashes.cmake`, `tests/diag/expected.csv`, `runner/audio.c`, `.github/workflows/ci.yml` | `fixtures.vdp_vtable.*`, `fixtures.pause_nmi.*` (incl. `.judge.*` and `.control.wrong_profile`), `runner.hashes_file`, `runner.audio_cadence`, hosted `hash-equality` | `cmake --workflow --preset ci`; D1 artifact download and `shasum`; two mutations | 8 of 8 hosted files identical; Pause-line and sample-rate mutations red |
| REG-03 | `runner/expect.c`, `runner/expect.h`, `runner/main.c`, `runner/audio.c`, `tests/audio/*.txt`, ledger audio rows | `runner.expect.audio`, `runner.hash_audio.fm`, `compat.fixtures` | three mutations; by-hand fm-only identity check | pass; fails on every negative control; fm separates rows |
| REG-04 | `scripts/compat-common.sh` (adopt), `scripts/library.cmake`, `scripts/library.sh`, `tests/library/tests.cmake`, `tests/compat/update.cmake`, `tests/roms/src/header.inc` | `library.synthetic`, `compat.public_update` | `ctest -R library.synthetic -V`; adopt-condition mutation | rows_added=4 ratchet=ok; held merge byte-identical; mutation red. Owner run carried (not_run) |

### Runs I performed (all from the repository root)

| Command | Result | Exit |
|---------|--------|------|
| `cmake --workflow --preset ci` | 652 tests plus 5 `release` tests, all passed | 0 |
| `cmake --workflow --preset hygiene` | 74 of 74 passed (`hygiene.tree` 20.6 s, `hygiene.analyzer` 11.4 s) | 0 |
| `sh scripts/hygiene.sh --tree`; `sh scripts/hygiene.sh --history origin/main..HEAD` | clean | 0; 0 |
| `cmake -DROOT=$PWD -P tests/cmake/budgets.cmake` | `repository: 860 check lines / 9402 src lines, strict one tenth` (limit 940; phase ceiling 906) | 0 |
| `ctest --test-dir build/ci -N` against `tests/inventory/ci.txt` | 657 registered (652 + 5), 657 non-comment inventory names, equal after `cut -f1 | sort`; `wasm32.txt` 615 and `nofp.txt` 24 | 0 |
| `quality.json` floors | `tests.ci` 654 (registered 657 minus 3), `tests.wasm32` 612 (615 minus 3); enforced by `policy.quality*` inside the `ci` workflow | 0 |
| `wc -c docs/*.md` | largest `docs/embedding.md` 36,379 bytes, then `docs/runner.md` 34,048 and `docs/conformance.md` 32,320; all under the 38,912 cap | 0 |
| `git diff --quiet origin/main -- include src` | no diff; `COGMASTER_API_VERSION` 14, behaviour 16, state format 16 | 0 |
| `git diff origin/main..HEAD -- tests/built/expected.txt tests/diag/expected.csv tests/audio tests/io tests/vdp tests/bus tests/ports \| grep -c '^-[^-]'` | 0 removed lines (additions only; existing hashes and pins unchanged) | prints 0 |
| `normalize` of origin/main ledger vs live ledger, whole row | 5 of 5 equal; `cmp` fixed point | 0 |
| mutation `src/vdp.c:151` `lines - 2u` to `lines - 3u`, rebuild, `ctest -R 'pause\|vdp_vtable\|runner.hashes\|io.pause'` | 7 failed (`vdp.pause_nmi`, five `fixtures.pause_nmi.mailbox.*`, `runner.hashes_file`); `git checkout src/vdp.c` | restored |
| mutation `runner/audio.c` PAL den 709379 to 709378 | `runner.audio_cadence`, `compat.fixtures`, `runner.hashes_file` fail; restored | restored |
| mutation `runner/expect.c` audio judge forced to pass | `runner.expect.audio` fails; restored | restored |
| mutation `tests/audio/transitions.txt` Japan pin extended | `runner.expect.audio`, `runner.hash_audio.fm`, `runner.hashes_file` fail; restored | restored |
| mutation `tests/compat/ledger.csv` one audio hash digit | `compat.fixtures`, `policy.compat_doc_fresh` fail; restored | restored |
| mutation `scripts/compat-common.sh` fm default to `"off"` | `compat.migration`, `compat.ratchet`, `compat.fixtures` fail; restored | restored |
| mutation `scripts/compat-common.sh` adopt condition `parameter!="boots@600"` removed | `compat.public_update` fails; restored, `compat.\|policy.compat\|library` 11 of 11 pass again | restored |
| `ctest -R library.synthetic -V` | `merge: rows_added=4 ratchet=ok page_bytes=5718`; `merge: held (page over the docs cap)` | 0 |
| `git status --short` after every restore and at the end (before this report) | empty | clean |
| `git diff -U0 origin/main..HEAD -- . ':!.planning'` grep for `REQ-ids`, `SEED-`, plan or phase numbers, `TBD\|FIXME\|XXX`, `/Users/`, `/home/`, mail addresses | no match | none |
| `git ls-files` for rom, bios, sms, gg, srm, sav, bin | only the manifest-listed `tests/vectors/z80/sst-sample.bin`; no added non-text file | none new |
| `git log --format='%ae %ce' origin/main..HEAD` | 52 commits, all the GitHub noreply identity | n/a |

### Hosted evidence (read with `gh run view` and `gh run download`)

| Run | Workflow | Head | Conclusion | Notes |
|-----|----------|------|------------|-------|
| https://github.com/szTheory/cogmaster/actions/runs/43111533403 | CI dispatch D1 | 1c720da | success | all jobs success (hygiene, nofp x2, asan, be32, build x6, perf, retroarch, wasm32, hash-equality, `CI required`); title, rehearsal, cleanup skipped as for any dispatch; `hashes: 8 identical files` |
| https://github.com/szTheory/cogmaster/actions/runs/43119601937 | CI pull request | b95e2ea | success, attempt 2 | `--failed` rerun for the known rehearsal-cleanup race; all 20 jobs success including `hash-equality`, `rehearsal`, `cleanup`, `CI required` |
| https://github.com/szTheory/cogmaster/actions/runs/43121365762 | CI pull request | ab08de1 | success, attempt 2 | same; the race was in `scripts/publish.sh` cleanup and is what the fix round addressed |
| https://github.com/szTheory/cogmaster/actions/runs/43126535159 | CI pull request | eed3e34 | cancelled | superseded by the push of 7ae2085 (concurrency); jobs that finished before the cancel (hygiene, nofp x2, `build (linux-x64)`, title) were success |
| https://github.com/szTheory/cogmaster/actions/runs/43126677381 | CI pull request | 7ae2085 (current head) | in progress at the time of writing | every lane job success: hygiene, nofp x2, asan, be32, perf, build x6, retroarch, wasm32, `hash-equality`; `rehearsal` still running (the job whose cleanup race the fix round bounds), then `cleanup` and `CI required` |

The earlier failed run 43115877197 (head 0a42f8e) failed `build (windows-arm64)` on the `compat.public_update`
timeout; commit 2104092 gave it room and runs 43119601937 and 43121365762 are green. The fix-round commits after
ab08de1 touched the runner (an unreachable audio branch and a console message), compat scripts, `publish.sh` and
tests; `src/` and `include/` are untouched, and every lane of the 7ae2085 run is green on them. The orchestrator
confirms the final state of run 43126677381 (`rehearsal`, `cleanup`, `CI required`).

### Anti-patterns

None blocking. No debt markers, no planning identifiers in code, test names or user docs.

Timeouts: the per-test comments settled from hosted per-test times (commit 56163b0) carry run IDs; the provisional
count changed by `-1` in `tests/compat/tests.cmake`, `-1` in `tests/runner/tests.cmake`, and `+1` each in
`tests/fixtures/tests.cmake` (`fixtures.fm_sweep_jp.pin`, "no hosted run of this test yet") and
`tests/hygiene/tests.cmake` (the new publish-delete test), both added by the fix round after the last settled
hosted run. They are provisional only where no settled run exists yet, as required.

Stale local artifact (not a finding): my `build/ci/fixtures/` holds an October `boot_to_play_gg.gg`, so a local
`hashes.txt` carries one fixture row more than a clean checkout; hosted lanes build from clean checkouts and
agreed on eight identical files.

## Carried hand-off (not a gap; recorded as the batched owner hand-off)

The real own-library run is not run (status `not_run`, `human_judgment: true`), because it needs the owner's
library, which the legal rule forbids reading. It is recorded in SEED-013: run `sh scripts/library.sh run` once with
the library and BIOS folders configured and paste the `library-judge` block (counts only). It is a batched,
identity-bound run on the owner's hardware, not a gate and not UAT; ROADMAP Phase 12 criterion 5 links the rows of
that run. `human_needed: false`: the owner verifies nothing by default (rule 6). The CI image proves the mechanism
(rows, ratchet, held merge, no leaked names) on synthetic images only, which SEED-013 and `docs/conformance.md` say
plainly.

Seeds for what was left out (SEED-012 hashless audio predicate, SEED-014 PAL and FM follow-ups, including the
review leftovers on tooling, per rule 7) carry their triggers; none changes a hash today.

## Advisory notes (no gap)

1. **Final hosted run.** Run 43126677381 on 7ae2085 was in progress when I wrote this (lane jobs green, `rehearsal`
   running). Both previous green PR runs needed a `--failed` rerun of that one job; the fix is a bounded read-back in
   `scripts/publish.sh`, covered locally by `hygiene.publish_delete` (hygiene workflow 74 of 74). The orchestrator
   confirms the result.
2. **`hygiene.tree` timeout under load** did not occur here (20.6 s); per rule 7 a tooling concern, not a gap.
3. **STATE.md runtime row.** The verify step's runtime and model row is for the orchestrator to add.

## Gaps Summary

No gaps. All four roadmap criteria, as amended by plan 11-08, and requirements REG-01 to REG-04 are backed by code
that exists, is wired and is exercised by tests I ran (ci workflow 652 plus 5 of 657, hygiene workflow 74 of 74),
with negative controls I broke on purpose and watched fail (Pause line, sample rate, audio judge, audio pin, ledger
hash, fm default, adopt condition). The migration holds whole-row on the five rows of the old ledger. Eight lanes
produced one hashes file in the D1 dispatch and the pull-request runs. `include/` and `src/` are byte-identical to
`origin/main`, the counted check code is 860 of 906, docs are under the cap, and the tree is clean. The only carried
item is the owner's own-library run, `not_run`, in SEED-013.

---

_Verified: 2026-11-12_
_Verifier: Claude (gsd-verifier)_
