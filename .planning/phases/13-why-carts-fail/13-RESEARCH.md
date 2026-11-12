# Phase 13: Why carts fail - Research

**Researched:** 2026-11-12
**Domain:** C runner observation mode, POSIX sh/awk output boundary, CMake script tests, repo policy checks (no external packages)
**Confidence:** HIGH for the code facts (every discrete value below was read this session), MEDIUM for cost and timeout estimates (one host, no CI run yet)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

Method: the owner's standing fan-out boilerplate was applied. Four advisor agents (plumbing, leak contract, cause
taxonomy, run protocol and release mechanics) each read the code, applied the role lenses and ran a self-adversarial
pass; the orchestrator then reconciled them and ran a cross-cutting adversarial pass. All areas were selected; the
owner's boilerplate asks for one coherent set of recommendations that is followed, so no per-area vote was held. The
owner's earlier decision stands: the agent runs the diagnostic in a shell where the owner passes `COGMASTER_ROM_DIR`
(REQUIREMENTS, scope decisions of 2026-11-12). Do not reopen it.

#### Diagnostic surface

- **D-01:** The per-cart facts come from a new read-only runner mode that prints exactly one closed-vocabulary line per
  cart. It is not a ladder of `boots@F` records parsed from observation text. Reason: the roadmap asks for "frames to the
  first non-blank sample", the boots predicate counts distinct crop hashes (saturating at two) and cannot tell a static
  picture from a blank backdrop with the display on, and the cause taxonomy needs a picture count and an extended
  1800-frame view; the ladder cannot give those. The plumbing advisor named this exact need as its tripwire for a runner
  change. — **Reversibility:** costly — a documented runner flag is a maintained user-facing surface (`--help`, `docs/runner.md`).
- **D-02:** The mode runs the cart for 1800 frames in one pass and reports its facts at frame 600 and at frame 1800. The
  frame-600 verdict is the same predicate as `boots@600` (display latch seen, at least two distinct crop hashes, no first
  fault), computed from the same state, so it reproduces the ledger outcome; the rest of the run only adds evidence. A
  test pins that the mode's frame-600 verdict equals the exit status of `--expect boots@600` on every fixture.
- **D-03:** Facts printed per cart (all numbers, enum names or static-table strings; no `%s` of file or header text):
  first display-latch frame; first non-blank frame (the crop holds two or more distinct RGB444 values; define it once in
  `docs/runner.md`); distinct pictures capped at 8, at 600 and at 1800; first fault kind and device, PC, a fault frame
  bucket (0, 1-9, 10-99, 100+) and whether the fault came before or after the first non-blank picture; the board the core
  chose and its source. Never printed: SHA-1 or SHA-256 of anything, opcode, line, dot, exact fault frame, image size,
  region guess, file names, paths, probe text, the probe's `title=`.
- **D-04:** If computing the first non-blank frame or the picture count needs a new public-header field, stop and report
  it; do not widen the ABI silently. The expectation (from the advisors' reading of `runner/expect.c`) is that the runner
  already holds the crop and the fault record. No `COGMASTER_ABI_VERSION` or behaviour-revision change is expected.
- **D-05:** The diagnostic is `sh scripts/library.sh diagnose`, a branch in the existing `library.sh` and
  `library.cmake`, not a new script (both are class `operation` in `tests/policy/repository-checks.txt`, so counted check
  code stays at 887 of 940). The branch is reached by changing the argument guard so `diagnose` is accepted, and it ends
  in `exit 0` before the ledger-merge block. It takes no ledger or page argument and cannot call update, ratchet or copy.
  It reads the tracked ledger and catalog read-only, for the totals block only.
- **D-06:** `diagnose` builds the runner itself (`cmake --preset library` then `cmake --build --preset library`, output to
  stderr) so counts match HEAD. `run` does not rebuild. Never use `cmake --workflow --preset library` or `sh scripts/library.sh`
  with no argument or `run` in this phase: the workflow's `library.local` test runs `run`, which overwrites the tracked
  `tests/compat/ledger.csv` and `docs/compatibility.md`.
- **D-07:** Rows run: every image whose catalog row gives a default NTSC pad/pad profile, not only the ledger's fail rows.
  Pass rows cost nothing extra and give the baseline that decides FIX-02 (when a passing cart first moves). Per-row lines
  print for fail rows only; pass rows appear as a histogram of first-moving frame per mapper, counts only. The derived PAL
  and Japanese FM profiles are not run (all 171 PAL fails are already NTSC fails).

#### Leak-proof output

- **D-08:** The output is built by parse-and-reconstruct. A validator in `library.sh` (awk) accepts a record only when
  every field matches a closed pattern and prints with `printf` from validated fields, never `$0` and never a pass-through
  of runner output. Anything else counts as `malformed`. The rule is an allowlist: log only what can be named in advance.
- **D-09:** Printed grammar (four anchored line shapes, a test asserts every stdout line matches exactly one):
  `library-diagnose 1` with `core=<short sha>[+dirty]` and the behaviour revision; per fail row
  `diag: title=<T> group=<leaf> first_display=<n|none> first_nonblank=<n|none> distinct600=<d> distinct1800=<d> fault=<kind|none> dev=<dev|none> pc=<hhhh|none> mapper=<name>`;
  `diag-group:` lines (count per leaf, descending, ties by precedence then kind code); and a totals block
  (`diag-other:` and the ledger cross-check in D-18). No date, path, timing, candidate index or directory-order value.
  Rows sort by title with `LC_ALL=C`.
- **D-10:** Titles come only from `catalog.tsv`, looked up by the 40-hex probe identity (`length()` check), never from
  the probe's `title=`. A title prints only if it matches `^[A-Za-z0-9][A-Za-z0-9 ().,'&:!+_-]*$` and is at most 80
  bytes; otherwise `title=-` and `title_withheld` counts it. An image not in the catalog is counted in `uncatalogued`
  and never named, hashed or sized.
- **D-11:** All per-cart scratch lives in `mktemp -d "$root/build/library/diag.XXXXXX"` removed by an EXIT trap, with
  `umask 077` and the existing inside-`build/` guard; no `.path`, `.probe`, `.error` or `.observation` file is written in
  diagnose mode (stale ones from earlier `run` calls are not read). The only retained artefact is
  `build/library/diag/diagnose.txt`, byte-equal to stdout. Runner and cmake stderr are discarded (`2>/dev/null`); the
  command's own stderr carries fixed texts only. Say in `docs/testing.md` that SIGKILL skips the trap.
- **D-12:** The agent reads only the command's stdout or `diagnose.txt`. It never `cat`s, lists, greps or globs `*.path`,
  `*.probe`, `*.error`, `*.judge.txt` or `*.observation` in `build/library/results/`.
- **D-13:** Test `library.diagnose` in a new `tests/library/diagnose.cmake` (at most about 120 lines, registrations at
  most about 25), separate from `library.synthetic` because that test already sits near its Windows ARM64 timeout (about
  80 s of 160 s). Register it in `tests/inventory/ci.txt` and `tests/inventory/wasm32.txt`; give its timeout a
  `# Timeout: N s; 2 x ...` provenance comment from live runs (`hygiene.timeout_comments`). If a second library-lane run
  shows the margin is fine, folding it back is Claude's discretion. Fixtures are owned: a never-draws cart (the existing
  halt image) with a canary byte string in its tail; `boot_to_play.sms` as a pass row that must not be printed; a catalogued
  fail cart whose catalog title is hostile (RLO, ESC built with `printf` octal escapes) expecting `title=-`; an
  uncatalogued image with its own name canary.
- **D-14:** Canaries are assembled by `string(CONCAT)` from fragments so the whole string never appears in a tracked
  file: a bytes canary in the image tail, a name canary and home-path fragment in the file name, a directory canary in a
  parent directory. The scan also computes the image SHA-1 and SHA-256 and the absolute output path (matched with
  `string(FIND)`, both slash forms, because `cygpath -m` rewrites the image path) and checks structure: no byte outside
  0x20-0x7E and LF, no `/` or `\`, no run of 8 or more hex digits, no `.sms` or `.gg`. Channels scanned: stdout, stderr,
  `diagnose.txt`, and every file under the diagnose output directories; the test asserts the retained file set is exactly
  `{diagnose.txt}`. A vacuity guard runs first and fails by name if a canary is not really present, the expected `diag:`
  and `diag-group:` lines are missing, or the pass title appears.
- **D-15:** Negative controls through `tests/cmake/expect_fail.cmake` with a pinned `REGEX` of the form
  `diagnose leak: <class>`: `leak_name`, `leak_path`, `leak_bytes`, `leak_digest` (the test appends the planted leak to
  captured output before the scan, proving the scan has teeth) and `leak_title` (the catalog fail title is set to the
  name canary, going through the real path; the title-subset check and name scan must both fire). A hostile-runner case
  uses the existing `COGMASTER_LIBRARY_RUN` and `COGMASTER_LIBRARY_EMULATOR=sh` seam with a wrapper that appends the image
  path and first 16 image bytes to the runner's stdout; output must be unchanged except `malformed` rising. No leak switch
  inside production code.
- **D-16:** Criterion 4 (ledger and page unchanged) is shown two ways: inside `library.diagnose`, hash the stand-in ledger
  and page before and after a diagnose call and assert equality, with the existing `run` over the same inputs as the
  control that does change them; and once on the real tree, `git diff --exit-code origin/main -- tests/compat/ledger.csv docs/compatibility.md`.
  No new counted CI gate for it. A second diagnose call over the small stand-in directory must give byte-equal output
  (fault prevented: a date, path or order dependence).

#### Cause groups and the note

- **D-17:** The taxonomy is fixed here, before the first run, and is not tuned after it. Facts per failing row: D = display
  latch seen, P = at least two distinct crop hashes, F = first fault present, all at frame 600. First match wins:
  1. `input_mismatch`: the board the core chose differs from the board the catalog row names (reported even if 0 rows).
  2. `fault_no_display`: F and not D.
  3. `fault_static`: F, D and not P.
  4. `fault_alive`: F, D and P (only the fault fails the row; a hypothesis, not proof it is harmless, because only the
     first fault is kept).
  5. `dark_no_fault` (not F, not D), split in order: `latch_miss` (a non-blank picture by 600 although the latch never
     fired), `late` (first non-blank picture by 1800), `never_draws` (none by 1800).
  6. `static_no_fault` (not F, D, not P), split in order: `animates_late` (two or more distinct pictures by 1800), then
     `frozen` or `alive_still` by the RAM-liveness boolean (see D-19).
  Leaf group = class plus split, and for classes 2-4 also fault kind x device. After the run a leaf of 5 or more may be
  split to expose mixed causes; leaves are never merged and never re-split to push them under 5.
- **D-18:** Fault PC is bucketed by Z80 slot (`$0000-$00FF`, `$0100-$3FFF`, `$4000-$7FFF`, `$8000-$BFFF`, `$C000-$FFFF`)
  and a fault address class (ROM window, slot 2, RAM, mapper registers `$FFFC-$FFFF`, or I/O port) as sub-rows under a leaf.
  An exact PC or port is shown, as a count, only when 5 or more rows share it. Two cross-cut tables, counts only and not
  disjoint: `board_source` x leaf, and mapper x class (this is how FIX-03 gets a cause for `zemina`, `korean_nb` and
  `korean` even when each lands under 5 in a leaf; FIX-03 does not depend on the 5-row threshold). Totals block:
  `judged`, `run_fail`, ledger default-profile fail rows at this commit, `matched`, `ledger_fail_not_in_run` (explains
  207 against 210; rows not in the run get no group), `ledger_fail_now_pass`, `run_fail_not_in_ledger`, `groups_total`,
  `fail_total`. The plan's acceptance asserts `groups_total == fail_total` by exit status, never on printed text.
- **D-19:** RAM liveness (a boolean from comparing a hash of `$C000-$DFFF` at two samples 60 frames apart, never printed)
  is included only if it costs a few lines in the runner mode. It is weak evidence (a counter in a jammed loop reads as
  alive; a title waiting for Start writes nothing) and is never the sole basis for loosening the check. A Start-press
  script run is out of this phase; Phase 14 may run it for the one group that needs it.
- **D-20:** The note is `.planning/phases/13-why-carts-fail/13-CAUSE-GROUPS.md`, written by the agent from the
  diagnostic's printed counts (copied from the command output, not retyped). Header: full core commit, behaviour revision,
  UTC date, the command and its exit status (rule 11), the commit of `tests/compat/ledger.csv` read, the two caps. Table
  columns: `rank | group | fault kind x device | rows | sub-rows`. Counts only: no titles, no file names, no per-row PC
  lists, no cumulative-percent column and no "would flip" projection (the requirements forbid a numeric target and a
  Pareto column invites one). One line says: "groups under 5: N groups, M rows". The note says Phase 14 needs the dumps
  again to reproduce one leaf (the owner passes `COGMASTER_ROM_DIR`), and that titles for a leaf come from filtering the
  `group=` field of the diagnostic's per-row lines in the agent's terminal.
- **D-21:** Adversarial constraints Phase 14 must inherit (record them in the note): group size is not cause validity;
  dropping the fault term would hide real mapper-range or ROM-write faults, so any benign fault kind is justified per kind
  from hardware documentation; the late-first-picture frame is chosen from the distribution of first-picture frames where
  a gap shows, never by how many rows it flips, and `never_draws` keeps failing (FIX-02's second fixture).

#### Run protocol, docs and release

- **D-22:** Environment: the owner launches Claude Code from a shell that exports `COGMASTER_ROM_DIR` (the agent's Bash tool
  does not carry exports between calls), or runs `! COGMASTER_ROM_DIR='<folder>' sh scripts/library.sh diagnose` in the
  session. `diagnose` starts with a preflight that exits 2 with a fixed text and never prints the value. On exit 2 the agent
  stops and asks for consent under rule 6, naming the cause and the exact command. The agent never names, reads or writes
  `.env.local` (the script's own literal read of it stays as it is) and never retries a guard-blocked command in another
  form; a denial is not a verification result. The owner's path is never written to a file or commit message.
- **D-23:** Evidence in the plan SUMMARY, per rule 11: the exact command, its exit status, `core=<short sha>`, and the
  printed `candidates/attempted/evaluated` and group counts copied verbatim; verification kind `other` for the real-library
  run and `integration` for the CI test with its run URL. No log is committed. The run is bounded by the runner's emulated
  frame cap and a per-call wall-clock backstop; a timeout counts as `infrastructure_error`. If the whole run exceeds the
  Bash tool limit, use `run_in_background`. No attempt quota, no retry protocol (rule 8).
- **D-24:** Docs within budgets: a paragraph (about 600 bytes) in `docs/conformance.md` (34,733 of 38,912) beside the
  library-judge text; the command and the `library.diagnose` entry in `docs/testing.md` (33,825); the new runner mode
  and the first-non-blank definition in `docs/runner.md` (34,048) and `--help`; three lines in `ASSET_POLICY.md`
  section 4 naming the allowed diagnostic fields and saying diagnose output is never committed per row. No new doc file, no
  README change.
- **D-25:** Release: the phase must cut a release (rule 10, DEC.11). The runner gains a documented flag, so the PR title is
  a `feat` with scope `runner` or `library` (for example `feat(runner): add a counts-only diagnose mode for the library
  judge`), which release-please turns into a minor bump (0.14.0). `docs`, `test` and `chore` titles cut no release and
  `scripts/check-title.sh` rejects `feat` with scope `ci`, `release`, `build`, `test` or `docs`. Keep `BREAKING CHANGE` text
  out of the body. The library and runner behaviour is unchanged, so the release carries a new runner flag and no core
  change. `release_inputs` stays false (no edit to `release.yml`, `ci.yml`, `publish.sh`, `release-please-config.json` or
  `cmake/packaging.cmake`).

### Claude's Discretion

- Exact field order and spacing inside the closed grammar, provided every line shape stays anchored and tested.
- Whether RAM liveness (D-19) ships, and the sample frames it uses.
- Whether `library.diagnose` can later fold into `library.synthetic` (D-13) once live runs show the margin.
- The leaf names beyond those listed, and the exact fault-address class boundaries.
- How the runner mode is spelled (`--diagnose` is the working name) and how its frame-600 verdict is shared with the
  `boots` code rather than copied.

### Deferred Ideas (OUT OF SCOPE)

- A Start-press scripted rerun to separate "waiting on input" from "frozen": Phase 14, only for the group that needs it.
- Per-instruction trace or CPU register output: Phase 14 uses `--trace` on one exemplar.
- A PAL or Japanese FM diagnostic: no value (PAL fails are NTSC fails).
- A CI job or counted "ledger and page unchanged" script, a library fingerprint, a second output format, attempt quotas: rejected as overbuilding (rule 7, rule 8).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DIAG-01 | Diagnostic mode reports, per default-profile fail row, frames to first non-blank, first fault kind and PC, mapper chosen; only counts, kinds, catalog titles | Runner already holds crop, fault record, board (D-04 holds, section "D-04 verdict"); `expect.c` split sketch; `library.sh` branch points; record grammar proposal |
| DIAG-02 | Synthetic-fixture test proves no image bytes or file names in output; negative control that leaks makes the test fail | Existing patterns: `library.synthetic` stand-in, `flaky-runner.sh` wrapper seam, `expect_fail.cmake`; owned fixtures verified to cover every leaf; CMake regex has no `{n}`; cost control for negative controls |
| DIAG-03 | Failing rows grouped by cause with counts in a planning note | Taxonomy completeness check, observable-fact audit, `compat-common.sh cases` as the one ledger reader, note acceptance by exit status |
</phase_requirements>

## Project Constraints (from CLAUDE.md)

Treated as locked, same authority as CONTEXT.md. Those that bind this phase:

- Rule 1, 3: no GPL source opened; no ROM/BIOS/save bytes, personal paths, e-mail addresses or names committed. Research files and the note must use repo-relative paths only (hygiene `HOME_PATH` pattern is `(/Users/|/home/)[A-Za-z0-9._-]+` over tracked files, including `.planning/`) [VERIFIED: scripts/hygiene.sh:13].
- Rule 4: deterministic, bounded core; integer-only; no wall clock. The diagnostic mode must be a fixed 1800-frame run with no time source.
- Rule 5: every behaviour change lands with a test that `cmake --workflow --preset ci` runs; README short; testing/CI docs in `docs/`; public header terse (this phase does not touch it).
- Rule 6: automation-first; ask the owner only for identity-bound consent or their own hardware (here: `COGMASTER_ROM_DIR`), and say why.
- Rule 7: check-code stays under one tenth of `src/` lines; every gate names the fault it prevents; code review before verification; at most two review fix rounds and one gap round.
- Rule 8: no attempt quotas, ceremonies, lifetime counters.
- Rule 9: one GSD step per session; never `/gsd-next`, `--auto`, `--chain`; record runtime and model in STATE.md at every phase step.
- Rule 10: never commit on `main`; `phase/NN-slug` branch off `origin/main`; Conventional Commit PR title; `gh pr merge --auto --squash` in a shell; confirm CI run on head and the release PR green.
- Rule 11: evidence is a command, an exit status and a run URL, never a committed log.
- Rule 12: pins public (not used here).
- Style: small modules, plain control flow, fixed-width ints; public names `cogmaster_`, runner-internal `cogmaster__`; hardware comments cite a source; no planning IDs in code, test names or user docs.
- Summary format: every plan SUMMARY has `requirements-completed:` and a block-style `coverage:` list; acceptance checks assert on exit status and tolerant regexes, never exact tool text.
- Dump handling (from the orchestrator): never read dumps; never run `sh scripts/library.sh` with no argument or `run`, nor `cmake --workflow --preset library`; the owner's `COGMASTER_ROM_DIR` is never printed or committed.

## Summary

The phase is buildable inside the CONTEXT decisions with one tripwire cleared: **D-04 holds**. Everything the diagnostic needs is already public API: the per-frame crop (`cogmaster_get_frame`), the sticky first-fault record including a 0-based `frame` (`cogmaster_get_fault`), the board and its source (`cogmaster_probe`, which `cogmaster_load` itself calls through `cogmaster__cart_probe`), and RAM bytes (`cogmaster_peek`). No header field, ABI version or behaviour revision changes. The runner already computes the `boots@F` state per frame in `cogmaster__expect_frame` (`runner/expect.c:140-204`): `display` OR-latch, `distinct` saturating at 2 against frame 1's hash, and `fault` OR-latch. The clean way to share the frame-600 verdict is to split that function into a printing-free `observe` step and a one-line `boots` predicate, and have the new mode call both.

Four CONTEXT statements are wrong or risky and should be corrected in the plan (details in "CONTEXT Risks"): (1) the Japanese-region default row is the *Japanese FM* profile (`sms|japan|ntsc|on`), 18 of the 210 default-profile fails live there, so "Japanese FM profiles are not run" must mean only the PAL profile is skipped; (2) `board_source` is always `db` for a catalogued cart (the catalog is generated from the same table the probe matches by SHA-1), so the `board_source x leaf` cross-cut and `input_mismatch` are structurally near-vacuous; (3) D-14's "no run of 8 or more hex digits" collides with `core=<short sha>` once git abbreviates to 8+ characters, so print exactly the first 7 characters; (4) D-03 prints no "first frame the picture changed" but D-07's pass-row histogram needs exactly that, so the runner line needs a `first_change` field.

Owned fixtures already cover the taxonomy, so the synthetic test needs no new binary builders: `di-halt.sms` (never draws, no fault), `in-a-00.sms`, `ld-8000.sms`, `ed-00.sms` (fault before any display, three kind x device pairs), `static-display.sms` (D, not P, no fault), `boot_to_play.sms` and `vdp_lines_192.sms` (pass), and the owned `mapper_korean.sms` / `mapper_hes_4pak.sms` fixtures, which are static pictures that **fail** `boots@600` on this commit. That last observation is direct, dump-free evidence for the "check too strict" hypothesis Phase 14 will test.

**Primary recommendation:** Add `--diagnose` as a runner mode in a new `runner/diag.c` that embeds `cogmaster__expect` and reuses a split `cogmaster__expect_observe` / `cogmaster__expect_boots`; run it only through `scripts/library.cmake` (which owns the `COGMASTER_LIBRARY_EMULATOR` list-splitting seam that the wasm32 lane needs); validate and reconstruct every record in awk using only features common to mawk, BWK awk and gawk; test it with one real scenario run plus cheap static-text negative controls through one shared scan file.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Per-frame observation (display latch, crop hashes, first non-blank, fault, RAM sample) | Runner (`runner/diag.c`, `runner/expect.c`) | Core public API (read-only) | Only the runner holds the per-frame loop and the crop; the core exposes it through the public header |
| Closed-vocabulary record line | Runner | Shell/awk validator | The runner emits only numbers, enum names and table strings; the validator is the second, independent allowlist |
| Profile choice, catalog join, title hardening, grouping, sorting | `scripts/library.cmake` (per image) + awk in `scripts/library.sh` (aggregate) | tracked catalog and ledger (read-only) | The judge already owns profile derivation (`library.cmake:139-152`); aggregation is text reduction |
| Leak guarantee | awk reconstruct (`printf` of validated fields) | `tests/library/diagnose.cmake` scan with canaries | Defence in depth: output construction cannot pass a byte through; the test proves it with teeth |
| Build of the runner at HEAD | `library.sh` (cmake preset `library`) | — | Counts must match HEAD; skipped when `COGMASTER_LIBRARY_RUN` is set (the test seam) |
| Cause note | Agent, from printed counts | — | Counts only, copied from the command output |
| Docs | `docs/runner.md`, `docs/conformance.md`, `docs/testing.md`, `ASSET_POLICY.md` | — | Within the byte budgets listed below |

## Standard Stack

No external package is added or needed. Everything is in-repo, and rule "another copy and paste beats another dependency" applies.

### Core
| Component | Version / state | Purpose | Why standard here |
|-----------|-----------------|---------|-------------------|
| `runner/expect.c` + `expect.h` | HEAD (216 + 27 lines) | boots predicate state | The one implementation of `boots@F`; reuse, do not copy (D-02) |
| `runner/args.c` | HEAD (377 lines) | option parse, `synopsis[]` | All options and `--help` text live here |
| `runner/main.c` `run()` | HEAD (760 lines) | the frame loop | Hook points at lines 512-520 (per frame) and 584-587 (end) |
| `runner/probe.c` `cogmaster__board_name` | HEAD | catalog word for a board | Already non-static, declared in `probe.h:26` |
| `scripts/library.sh`, `scripts/library.cmake` | HEAD (92 + 189 lines) | the library judge | `operation` class, uncounted |
| `scripts/compat-common.sh cases` | HEAD | the one quoted-CSV ledger reader | Do not `awk -F,` the ledger: titles hold commas |
| `tests/cmake/expect_fail.cmake` | HEAD (12 lines) | exact-status negative controls | Pinned `REGEX`, exit code `1` for a CMake `FATAL_ERROR` |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| New `runner/diag.c` | Everything inside `expect.c` | `expect.c` is a 216-line focused file; the diagnostic adds roughly 120 lines and a different lifetime. A new file also needs one line in `COGMASTER_RUNNER_SOURCES` (`CMakeLists.txt:141-158`), which the test runners (`cogmaster-test-run-stop`, `cogmaster-test-run-audio-short`) reuse automatically [VERIFIED: tests/runner/tests.cmake:1808,1816 use `${COGMASTER_RUNNER_SOURCES}`] |
| Do the whole per-image loop in `library.sh` | Keep it in `library.cmake` | CMake `execute_process(COMMAND ${emulator} "${run}" ...)` splits the `;`-list emulator (wasmtime plus flags) into argv; POSIX sh word splitting would not [VERIFIED: tests/library/tests.cmake:94-98 pass the list inside one `COGMASTER_LIBRARY_EMULATOR=` value; scripts/library.cmake:12-13,63 expand `${emulator}` unquoted] |

**Installation:** none. **Package Legitimacy Audit:** not applicable, this phase installs no external package. Packages removed due to `[SLOP]`: none. Flagged `[SUS]`: none.

## Architecture Patterns

### System Architecture Diagram

```
owner shell (COGMASTER_ROM_DIR) --> sh scripts/library.sh diagnose
   |  preflight: dir exists? else exit 2 fixed text
   |  build: cmake --preset library; cmake --build --preset library   (skipped if COGMASTER_LIBRARY_RUN set)
   |  mktemp -d build/library/diag.XXXXXX  (trap rm -rf EXIT, umask 077)
   |  catalog read-only; ledger read-only via compat-common.sh cases (totals block only)
   v
 for each regular .sms/.gg/.bin/.rom in dir (flat, byte-order glob, no symlinks)
   |  env: COGMASTER_IMAGE, COGMASTER_DIAGNOSE=1  ->  cmake -P scripts/library.cmake   (no .path/.probe/.error written)
   |     1. ${emulator} ${run} --probe IMAGE      -> OUTPUT_VARIABLE (never a file)
   |     2. judged id = identity_sha1 else sha1 / sha1_headerless; exact catalog match, 40 hex
   |     3. profile as library.cmake:139-152 (first, NTSC, pad/pad); skip PAL; skip catalog status != unknown
   |     4. ${emulator} ${run} --diagnose --machine .. --region .. --fm .. --video-mode .. --port1 pad --port2 pad IMAGE
   |            -> runner: 1800-frame loop, one line on stdout, stderr discarded (ERROR_QUIET)
   |     5. append "<judged> <machine> <profile> | <runner line>" to $tmp/records     (scratch only)
   v
 awk validator (LC_ALL=C): every field matches a closed pattern or the record counts as malformed
   |   join title from catalog by 40-hex id (80 bytes, charset subset) else title=-
   |   classify (D-17), bucket PC/class, count leaves, histogram pass rows by mapper
   |   printf from validated fields only; sort with LC_ALL=C sort
   v
 stdout == build/library/diag/diagnose.txt  (exit 0; nonzero only if groups_total != fail_total, after printing)
```

### Recommended Project Structure
```
runner/
├── diag.c / diag.h     # new: cogmaster__diag state, per-frame step, one-line print (~120 lines)
├── expect.c / expect.h # split: cogmaster__expect_observe + cogmaster__expect_boots
├── args.c / args.h     # --diagnose option, usage line, combination checks
└── main.c              # 4-6 lines: init, per-frame call, print at end
scripts/
├── library.sh          # guard -> case run|diagnose; diagnose branch ends in exit before the merge block
└── library.cmake       # early COGMASTER_DIAGNOSE block, return() before the .path write (line 7)
tests/
├── library/diagnose.cmake        # new: script mode (real run + scans); static controls call the same scan
├── library/tests.cmake           # registrations (inside the existing if(COGMASTER_FIXTURES))
├── runner/tests.cmake            # runner.diagnose.* (usage, verdict equals --expect, line shape)
└── inventory/{ci,wasm32}.txt     # one row per new test, sorted
```

### Pattern 1: share the boots@600 state, do not copy it (D-02, answers Discretion item 5)

**What:** `cogmaster__expect_frame` (`runner/expect.c:140-204`) interleaves three jobs: observe the frame (lines 144-177), judge accepted cases, and print. Split the first and last into reusable pieces. The state to share is exactly the struct in `runner/expect.h:11-15`:

```c
typedef struct cogmaster__expect {
    cogmaster__expect_case cases[COGMASTER__EXPECT_MAX];
    uint32_t count, display, distinct, fault;
    char first[65];
} cogmaster__expect;
```

and the predicate at `runner/expect.c:196`: `(uint32_t)(e->display && e->distinct >= 2u && !e->fault)`.

**When to use:** always for D-02; this makes "same predicate, same state" true by construction and leaves the existing `predicate ... observed_frame=` output byte-for-byte unchanged (the existing test `runner.expect.predicates` pins `display=0 distinct_at_least=1 first_fault=0` text, `tests/runner/expect.cmake:61`).

**Example (sketch, not final code):**
```c
/* expect.h */
/* Runs one frame's observation: crop hash into hex, and the display, fault and distinct facts. blank is 1
   when no pixel differs from the first, the picture the diagnostic calls blank. Returns an exit status. */
int cogmaster__expect_observe(cogmaster__expect *e, const cogmaster *m, uint32_t display,
                              uint16_t *pixels, char hex[65], uint32_t *blank);
/* boots@F: a display latch, two distinct crops, no first fault. */
int cogmaster__expect_boots(const cogmaster__expect *e);
```
`cogmaster__expect_observe` is lines 144-177 with one extra comparison inside the existing pixel loop (compare each `v` to `pixels[0]`); `expect_frame` calls it, then keeps the case loop, replacing line 196's inline expression with `cogmaster__expect_boots(e)`.

The diagnostic struct embeds the shared state and adds only what boots does not track:

```c
typedef struct cogmaster__diag {
    cogmaster__expect e;                  /* the boots@600 state, same code path as --expect */
    uint32_t first_display, first_nonblank, first_change; /* 1-based run call, 0 = none */
    uint32_t seen, seen600, boots600;     /* distinct hashes held (cap 8), at 600, verdict at 600 */
    char hash[8][65];
    uint32_t ram1740, ram_live;           /* D-19: hash of $C000-$DFFF at two samples 60 frames apart */
} cogmaster__diag;
```

### Pattern 2: closed-vocabulary runner line

Proposed single stdout line (field order is Claude's discretion; every value is a number, an enum name or a table string):

```
diagnose 1 verdict600=fail first_display=none first_nonblank=none first_change=none distinct600=1 distinct1800=1 ram=still fault=unmapped_read dev=ctrl pc=0002 where=port fault_frame=0 fault_order=nopicture mapper=sega source=db
```

Reuse, do not retype, the tables: `print_fault` (`runner/main.c:220-248`) holds the kind and device names in function-local `static const char *const` arrays; extract them into two functions beside `cogmaster__board_name` and call them from both `print_fault` and the diagnostic. Add a board-source name function with the same table `probe.c:190` already has locally (`{"none", "db", "header", "heuristic", "default"}`).

Exit status stays 0 on any completed run (77 refused and 1 failure as today); do not add a new status, because `docs/runner.md` and `--help` repeat the table and `runner.exit.table` keeps them equal [VERIFIED: runner/exit_status.h:1-4, tests/runner/tests.cmake:1985-1989].

### Pattern 3: option wiring (answers the `--expect` convention question)

`args.c` rules to follow: options parse into `cogmaster__run_options`; every combination rule lives in `check_combination` (`args.c:211-247`); the `synopsis[]` literal (`args.c:13-41`) is both `--help` and the usage text. For `--diagnose`:

- add `int diagnose;` to `cogmaster__run_options` (`args.h:14-41`);
- in `check_combination`: `--diagnose` needs a file; a usage error with `--frames`, `--bench`, `--probe`, `--expect`, `--hash`, `--hash-audio`, `--png`, `--trace`, `--mailbox`, `--eyes`, `--input`, `--save-dir`, `--boot`; then set `o->frames = 1800` (a named constant shared with the print code). Setting frames after the check avoids the order-dependent "give --frames or --bench once" path at `args.c:266-268`;
- allow `--machine --region --fm --video-mode --port1 --port2` (the library judge needs them);
- add one `--diagnose` line to `synopsis[]`, one row to the `docs/runner.md` options table (`docs/runner.md:64-90`), and a short section after "Accepted expectations" (`docs/runner.md:365-405`) that defines "first non-blank" once.

Existing tests do not pin the full help text, only that it exits 0 and lacks `--capabilities` (`tests/runner/tests.cmake:1161-1164`) and the exit table (`runner.exit.table`), so `--help` changes cost no test edits [VERIFIED by grep: no test matches the synopsis text].

### Pattern 4: `library.sh` branch (answers item 2)

Current guard, verbatim [VERIFIED: scripts/library.sh:11-13]:
```sh
if [ "${1-}" != run ]; then
  exec cmake --workflow --preset library
fi
```
Change to a `case` so only `run` and `diagnose` pass and any other argument still reaches the workflow as today. The reusable prefix is lines 14-22 (`out` default, the inside-`build/` guard, `mkdir`, physical-path guard, `chmod 700`, `umask 077`). Diagnose must not use the default `out` (`build/library/results`): give it its own default (for example `${COGMASTER_LIBRARY_OUT:-$root/build/library/diag}`) so stale `run` files in `results/` are never read, and keep the same guard text so the test can point it into its own build directory.

The `diagnose` branch must come after line 22 and before line 24-30 (ledger/page/cases), and must `exit` itself, so the merge block (lines 54-92) is unreachable from it. Structure it as a function-free block ending in `exit "$rc"`.

Preflight (new, none exists today; `run` with an unset variable dies under `set -u`): `[ -n "${COGMASTER_ROM_DIR-}" ] && [ -d "$COGMASTER_ROM_DIR" ] || { echo 'library: diagnose needs COGMASTER_ROM_DIR to name an existing directory' >&2; exit 2; }`. The fixed text carries no value. Note the existing `.env.local` literal read (`library.sh:6-10`) runs before the guard; leave it as is (D-22).

### Pattern 5: `library.cmake` early block

`library.cmake:7` unconditionally runs `file(WRITE "${prefix}.path" "${image}")` and line 13 writes `.probe` and `.error` via `OUTPUT_FILE`/`ERROR_FILE` [VERIFIED: scripts/library.cmake:7,13-14]. D-11 forbids those files in diagnose mode, so the diagnose logic must be an early `if("$ENV{COGMASTER_DIAGNOSE}")` block that ends in `return()` before line 6. Capture the probe with `OUTPUT_VARIABLE` and `ERROR_QUIET`, never to a file. The probe keys to extract are the same as lines 27-30: `machine_guess region_guess mapper_guess identity_sha1 sha1 sha1_headerless`. `judged` is `identity_sha1`, else `sha1`, else `sha1_headerless` when present (lines 111-117). Do not treat a catalog row as a CMake list (titles are free text; use `MATCHES` only, as lines 139-141 do).

Profile derivation to mirror (lines 139-152): `gg` gives `gg|export|ntsc|off|ntsc-u`, or `gg|japan|ntsc|off|ntsc-j` when `region_guess` is `ntsc-j`; `sms`/`sms1` with `ntsc-u` gives `sms|export|ntsc|off|ntsc-u` (plus a PAL profile that diagnose skips); `sms`/`sms1` with `ntsc-j` gives `sms|japan|ntsc|on|ntsc-j`. The video mode is `gg` for `gg`, else `sms`; controllers are `--port1 pad --port2 pad`.

### Anti-Patterns to Avoid
- **Parsing `--expect` observation text for the diagnostic.** The `predicate` line carries `observed_frame=<sha256>` (`expect.c:197-201`) and the fault line carries `op=$XX`, a ROM byte (`main.c:244-247`). Neither may reach the validator.
- **`awk -F,` over `tests/compat/ledger.csv`.** Quoted titles hold commas and quotes; use `sh scripts/compat-common.sh cases LEDGER`, whose output lines are `set(compat_N_<field> [[value]])` for `id machine region tv mapper predicate status ... config cap video fm port1 port2 script` (title and evidence are deliberately omitted) [VERIFIED: scripts/compat-common.sh cases branch, `names`/`parts` arrays].
- **Per-cart scratch files under `results/`.** Use only the `mktemp -d` directory and an EXIT trap.
- **A negative control that re-runs the whole scenario per class** (see "Test design").

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Quoted-CSV ledger parse | A second CSV parser in awk | `sh scripts/compat-common.sh cases` | It is the single bounded parser (4096-byte lines, 8193-row cap, quote and control-character checks) |
| Board and fault names | Hard-coded strings in the new code | `cogmaster__board_name`, extracted fault kind/device functions | One table per concept (`probe.c:142-148`, `main.c:222-236`) |
| SHA-256 of the crop | A second hash for picture counting | the digest `cogmaster__expect_observe` already produces | Same bytes, same endianness serialisation as `--hash` |
| Test-failure assertion with a pinned message | Custom pass/fail plumbing | `tests/cmake/expect_fail.cmake` with `-DCODE=1 -DREGEX=...` | Rejects "any failure"; `ASAN_OPTIONS` handling included |
| Registration and inventory | Ad hoc `add_test` | `cogmaster_add_test` (scales timeouts) + inventory rows | `cmake/CogmasterTests.cmake` fails configure if registered tests differ from the sorted inventory |
| Short git commit id | `git rev-parse --short` | first 7 characters of the full id (`cut -c1-7`) | `--short` length grows with the object count and breaks the 8-hex-digit scan (see Pitfall 3) |

**Key insight:** this phase's value is the *boundary*, not the computation. Every place a byte of an image, a name or a path could cross (probe stdout, runner stdout and stderr, cmake error text, scratch files, titles) must be closed by construction, then shown closed by a test that can fail.

## CONTEXT Risks and Corrections

These are the places a CONTEXT assumption is wrong, vacuous or underspecified. Each has a recommended resolution inside the locked decisions.

| # | Finding | Evidence | Recommendation |
|---|---------|----------|----------------|
| R1 | **D-07 drops real fail rows.** The ledger's default NTSC row for a Japanese-region Master System cart is `sms\|japan\|ntsc\|on`. By status: 18 fail, 30 pass (pad/pad) in that profile; `gg\|japan\|ntsc\|off`: 2 fail. D-07's "Japanese FM profiles are not run" would skip them, so the run would see about 190 of the 210 default-profile fails. | Ledger read through a parser this session: `('sms','japan','ntsc','on','pad/pad','fail')` 18; `('gg','japan','ntsc','off','pad/pad','fail')` 2; 50 catalog ids carry exactly one row, the Japan FM one. `library.cmake:150` derives `sms\|japan\|ntsc\|on` as the only NTSC profile for `ntsc-j` carts | Run exactly the first (NTSC) profile `library.cmake` derives, and skip only the PAL profile. Reword D-07 in the plan: "PAL profile is not run". |
| R2 | **`board_source` x leaf (D-18) and `input_mismatch` (D-17 class 1) are structurally near-vacuous.** A cart is "catalogued" only when its SHA-1 matches a catalog row; the catalog is generated from `src/cartdb.inc`, and the probe matches the same whole-image SHA-1, giving `board_source = DB` and `board = catalog board`. | `cogmaster__cartdb_image` hashes the body and `cogmaster__cart_fact_to_info` sets `out->board_source = COGMASTER_BOARD_SOURCE_DB;` (`src/cartdb.c:40-49`, `src/cart.c:306`); catalog board column values equal `cogmaster__board_name` words (checked: sega, none, codemasters, korean, korean_nb, hicom, zemina, nemesis, sega_eeprom, hes_4pak) | Keep both (they are cheap and the totals block can assert `source=db` for every fail row, which makes a silent catalog/DB drift visible), but do not spend plan tasks on a rich `board_source` cross-cut. The cross-cut that carries information is **mapper x class**. |
| R3 | **"Fault present at 600" is not the same as "a fault was recorded".** The core keeps only the first fault, for the whole run (`src/fault.c:9`). A cart whose first fault happens at frame 700 has F false at 600 but a fault record at 1800; D-09's `fault=` field would show it on a `dark_no_fault` or `static_no_fault` row, and D-03's buckets (0, 1-9, 10-99, 100+) cannot tell frame 150 from frame 700. | `fault.frame` is "counted from 0 at load" (`docs/runner.md:224`; `src/bus.c:330` `f.frame = m->frame`, `m->frame++` at `src/bus.c:102` after the frame runs). F at 600 is exactly `present && frame <= 599` | Compute `F600 = present && frame < 600` in the runner, group by `F600`, and add a `600+` bucket (`fault_frame=600+`) so a late fault is visible rather than contradicting the group name. |
| R4 | **D-14's "no run of 8 or more hex digits" collides with `core=<short sha>`.** `git rev-parse --short` returns 7 characters in this repository today (2,786 loose plus 6,559 packed objects) but lengthens as the repository grows or a path needs disambiguating. | Observed `987b011`; `git count-objects -v` | Print `core=` as the first 7 characters of the full id with `cut -c1-7` (7 < 8), and keep the scan threshold at 8. Real titles may also hold 8 digits (for example a date in parentheses; one catalog title does), so the structural scan is a test-only check over synthetic titles, never a production rule. |
| R5 | **D-03's fact list lacks the frame the picture first changes**, which D-07's pass-row histogram ("first-moving frame per mapper") and FIX-02's cap-versus-sound-check question need. `boots` only keeps "distinct reached 2". | `cogmaster__expect.distinct` saturates at 2 with no frame (`expect.h:11-15`) | Add `first_change=<n\|none>`: the first run call whose crop hash differs from frame 1's (the exact event that sets `distinct=2`, so it is the same state, plus a frame number). Histogram buckets are a plan choice, counts only. |
| R6 | **D-10's title charset withholds about 3% of real titles.** 56 of 1,677 catalog lines fail the regex or the 80-byte cap (4 are over 80 bytes). Main causes: `~` (13 characters), `?` (14), accented letters, `/`, `"`. | Computed over `tests/compat/catalog.tsv` this session | Legal under the locked decision (`title=-` plus `title_withheld`). The agent loses names only for those rows; the note carries no titles anyway. If Phase 14 needs a name, the `group=` filter plus the title-less count still identifies the leaf. Do not widen the charset without a reason. |
| R7 | **D-06 as written would reconfigure and rebuild inside the test.** `cmake --preset library` also runs `CMakeLists.txt:222-225`, which fails configure unless `COGMASTER_ROM_DIR` is an existing directory, and `build/library` is the owner's tree. | `CMakeLists.txt:223-225` | Rebuild only when `COGMASTER_LIBRARY_RUN` is unset. The test always sets it, so `library.diagnose` never configures or builds. |
| R8 | **Skipped and unsupported catalog rows would be run and then counted as "fail not in ledger".** `library.cmake` runs them and its update guard discards the facts; the ledger holds 12 `skipped` rows. | Ledger status counts; `library.cmake:182` still appends facts for them | Diagnose only catalog rows whose status column (field 6) is `unknown` and whose ports (fields 9 and 10) are `pad`/`pad`; count the rest in `skipped_excluded`. This also removes the `nonpad` ledger look-up (`library.cmake:129-137`) from the diagnostic. |
| R9 | **"207 against 210" is a candidate-level versus row-level count, not a bug.** The first library run printed `fail=207` of 603 judged candidates; the ledger holds 210 default-profile fail rows (a candidate can own more than one row; 18+2 Japan rows are among them). | `SEED-016-failing-carts.md:15`; ledger | Keep the totals block as designed; do not assert 207 or 210 anywhere (rule 8, requirements forbid numeric targets). |

### D-04 verdict (answers item 1)

D-04 holds. Mapping of each wanted fact to the existing public surface [all VERIFIED by reading the files cited]:

| Fact | Source today | Note |
|------|--------------|------|
| display latch, first display frame | `cogmaster_run_result.display_enabled` (`include/cogmaster.h:198-205`), set from `m->vdp.display` (`src/frame.c:95`) | sampled at the end of each run call, so a display switched on and off inside one frame is missed (this is what `latch_miss` names) |
| crop, first non-blank, distinct hashes | `cogmaster_get_frame` (`runner/expect.c:154`); `info.crop_w` pitch | compare each pixel to `pixels[0]` in the existing hash loop |
| first fault, kind, device, PC, addr, frame | `cogmaster_get_fault` (`cogmaster_fault`: `present, frame, line, dot, pc, addr, opcode, width, device, kind`, `include/cogmaster.h:223-231`) | sticky; one read after the last frame is enough |
| board chosen and its source | `cogmaster_probe` on the same image; `cogmaster_load` uses `cogmaster__cart_probe` (`src/instance.c:177`, `cart_copy` at `:260`) so probe and load agree | `main.c:429-431` already calls `cogmaster_probe` in the expect path |
| RAM liveness | `cogmaster_peek(m, COGMASTER_SPACE_BUS, addr)` | 8192 peeks at two frames is negligible beside a frame; mask the result to a byte (it returns `0x100` when unloaded) |

Off-by-one conventions to fix in the plan (and in `docs/runner.md`): the runner's call index `n` is 1-based; `fault.frame` is 0-based and equals `n - 1` of the call it happened in. So "fault at or before the first non-blank picture" is `fault.frame + 1 <= first_nonblank`. Name the order values `before`, `after`, `none` (no fault) and `nopicture`.

### Address class, from `device` first (answers item 4, fault boundaries)

D-18's class list ("ROM window, slot 2, RAM, mapper registers `$FFFC-$FFFF`, I/O port") is not exhaustive for the mapper: the Codemasters board's registers sit at `$0000`, `$4000` and `$8000` (`src/mapper.c`, `(addr & SLOT_MASK) == 0u && addr < 0xC000u`), and `mapper_range` is raised from `bank_write` / `ext_check` for any register address. Derive the class from `(device, addr)` in the runner, not from the address alone [VERIFIED: src/mapper.c:43,112,126,140; src/mapper_ext.c:34,102-193; src/io.c:463,508; src/bus.c:185,255,260,352; src/vdp_render.c:189]:

| device | kinds seen | Class to print |
|--------|-----------|----------------|
| `cart` | `unmapped_read`, `unmapped_write`, `rom_write` | `rom` when `addr < 0x8000`, else `slot2` |
| `ram` | `unmapped_read`, `unmapped_write` | `ram` (work RAM off) |
| `mapper` | `mapper_range` | `mapper` (register write selecting a bank past a non-power-of-two image) |
| `ctrl` | `unmapped_read` | `port` (`addr` holds the port) |
| `none` | `unmapped_write` | `port` |
| `cpu` | `ed_undefined` | `cpu` |
| `vdp` | `unsupported_video_mode` | `vdp` |

The PC slot bucket (D-18) is a pure function of `pc`; have the runner print a slot index (`pcslot=0..4`) in addition to or instead of the four hex digits, so awk never parses hex (mawk has no `strtonum`; `compat-common.sh` carries its own `hex()` helper for that reason).

### Taxonomy check (D-17..D-19 against what is observable)

- **Complete and exclusive.** With F600, D, P all at frame 600, the failing set is exactly F, or not D, or not P. Cases: F with (not D) is class 2; F, D, not P is class 3; F, D, P is class 4; not F, not D is class 5; not F, D, not P is class 6; not F, D, P is a pass. No fail row is unclassified, so `groups_total == fail_total` is achievable. Let the awk END block exit non-zero after printing if they differ (an assertion by exit status, as D-18 asks), without changing stdout.
- **`latch_miss` is real but narrow.** With the display never enabled the VDP draws only the backdrop, so a non-blank picture without a latch needs the display switched on and off within a frame (fade, loading screens). Keep the leaf; expect a small count.
- **`distinct >= 2` is weaker than "non-blank".** Two single-colour frames (backdrop changes from black to blue, display off) satisfy P and never produce a non-blank picture. The diagnostic exposes both facts, which is the point of D-01.
- **Owned fixtures show the strictness case without a dump.** On this commit `mapper_korean.sms` and `mapper_hes_4pak.sms` run the right mapper and display a static picture; they print `display=1 distinct_at_least=1 first_fault=0` and fail `boots@600` (measured with the existing `--expect`). They are class 6 (`static_no_fault`). This is evidence for D-21's "group size is not cause validity".
- **RAM liveness (D-19) is cheap.** Sample at frames 1740 and 1800; hash `$C000-$DFFF`. Cost is on the order of 16K peeks per cart against 1800 frames of emulation (a rendering cart takes about 1.5 s natively on this host; see Environment). Ship it.
- **Z80 slot bucket boundaries (D-18) are arbitrary but fine.** The only fault PC values the core records are instruction starts (`m->bus.op_pc`, `src/bus.c:344`; `ed_undefined` uses `pc - 2`, `:352`).

## Common Pitfalls

### Pitfall 1: a path or byte reaching stdout through an error channel, not the record
**What goes wrong:** the runner prints `cannot read <path>` and `unsupported: <path> is larger than 64 MiB` to stderr (`runner/probe.c:49,56`); `cmake -P` prints failing script lines; `--probe` prints `title=%s` and `unsupported_reason=%s` (`probe.c:221-222,230`); `--expect`'s fault line prints `op=$XX`.
**How to avoid:** `ERROR_QUIET` on every `execute_process`, `>/dev/null 2>&1` on the per-image `cmake -P`, and the validator reads only the runner's own closed line. The probe's `title=` is never read (D-10).
**Warning signs:** any file other than `diagnose.txt` under the diagnose directory after a run; the retained-set assertion in D-14 catches it.

### Pitfall 2: awk portability across the CI matrix
**What goes wrong:** CI runs ubuntu-26.04 (x64 and arm64), macOS 15 (arm64 and Intel), Windows (Git for Windows `sh`), plus qemu-ppc and wasm32 lanes that still use the host `sh` and `awk` [VERIFIED: .github/workflows/ci.yml:51,82,126-131,309,353]. This host's BWK awk is version 20200816 [VERIFIED: `awk --version`].
**How to avoid:** copy the repository's own awk dialect: no `{n,m}` interval expressions (spell out the character class; `compat-common.sh` does), no `strtonum`, `gensub`, `asort`, `PROCINFO`, `length(array)` on older awks, no `-v` carrying untrusted text, `sub(/\r$/,"")` on every input line (Windows), `LC_ALL=C` already exported at `library.sh:4`, `sort` with explicit keys (`-k1,1nr -k2,2n -k3`) for descending counts with a stable tie rule. The single quote in the D-10 charset must be written `\047` inside the single-quoted awk program. Interval support in mawk 1.3.4 builds from early 2020 is `[ASSUMED]` absent; avoid regardless. Avoid empty-regex edge cases on titles (`length($0)` in a C locale counts bytes, which is what the 80-byte cap needs).
**Warning signs:** a leaf count that differs between the Linux and macOS test legs.

### Pitfall 3: structural scan versus legitimate content
**What goes wrong:** the scan rules of D-14 (no byte outside 0x20-0x7E and LF, no `/` or `\`, no `.sms`/`.gg`, no run of 8 or more hex digits) false-positive on `core=<abbrev>` (R4), on `pc=` values, and on a synthetic title that happens to contain 8 digits.
**How to avoid:** `cut -c1-7`; keep synthetic titles like `Synthetic never draws`; the CMake regex engine has no `{8}` interval, so spell the run out as eight `[0-9a-fA-F]` atoms. [VERIFIED locally: `cmake -P` with `"xaay" MATCHES "a{2}"` reports no match on CMake 4.4.3; `"a{2}" MATCHES "a{2}"` matches the literal text.]

### Pitfall 4: binary and CRLF bytes in a captured runner stream
**What goes wrong:** the hostile-runner wrapper (D-15) appends 16 image bytes; CMake `OUTPUT_VARIABLE` and shell `$(...)` are unreliable for NUL, and the Windows runner prints CRLF.
**How to avoid:** make the canary tail printable (the existing halt image is the byte `v` repeated, a Z80 HALT, `tests/library/tests.cmake:11`); add `sub(/\r$/,"")` in the validator; write records with `file(APPEND)` and read them only with awk. For the wrapper use `dd bs=16 count=1 2>/dev/null` rather than `head -c` (not POSIX).
**Warning signs:** `malformed` not rising in the hostile case, or rising by a different count on Windows.

### Pitfall 5: the image size accidentally becomes a copier header
**What goes wrong:** an image whose length is 512 past a multiple of 16 KiB is read as headered (`n % 16384 == 512`, `runner/probe.c:189`), changing the probe identity path.
**How to avoid:** the halt image with a canary tail is 32768 plus the canary length; choose a tail length other than 512 (the precedent `"${halt}x"` is 32769 bytes, `tests/library/tests.cmake:210`). Because the length is no longer a power of two the board is the scan default (`board_source=default`), which is fine.

### Pitfall 6: negative controls that multiply runner time
**What goes wrong:** D-15 lists six controls; if each re-runs the full scenario (4 to 5 carts at 1800 frames each) the test runs about 30 emulations on every leg, including qemu-ppc and ASAN (timeout scale 3 and 8) [VERIFIED: CMakePresets.json:62,87 scale values; inventory `ci` applies to `ci`, `asan`, `be32` and MSVC lanes].
**How to avoid:** see "Test design": one real scenario, static-text controls for the four classes that need no real path (`leak_name`, `leak_path`, `leak_bytes`, `leak_digest`), real runs only for `leak_title` and the hostile runner, each over a one-cart directory.

### Pitfall 7: doc and policy tripwires
- `claims_scoped` fails if any `docs/*.md` contains the word "experimental" once the libretro `.info` flag is gone; it is gone now (`grep -ci experimental libretro/cogmaster_libretro.info.in` returns 0, and no doc holds the word) [VERIFIED: tests/cmake/repository.sh `claims_scoped`; grep this session]. Do not write it in the new doc text.
- `single_version` fails if a tracked file outside `version.txt`, `CHANGELOG.md`, release-please files and `.planning/*` holds the current version or the next minor (when `NEXT=ON`) as a literal [VERIFIED: tests/cmake/repository.sh:103-112]. Docs and test files must not name a version; the PR title example in CONTEXT is fine because it is not a file.
- Hygiene rejects home-path patterns and e-mail addresses in tracked text [VERIFIED: scripts/hygiene.sh:13-16]. The existing canary pattern assembles them with `string(CONCAT)` from fragments; copy it exactly.
- Rule 5 style: no planning IDs (`DIAG-01`, `D-07`) in code, test names or user docs.

### Pitfall 8: expect-file caps are irrelevant to the new mode, but the old limits still matter for the equality test
`--expect` accepts 1 to 64 records (`COGMASTER__EXPECT_MAX 64u`, `runner/expect.h:5`), at most 255 bytes each, frames at most 1,000,000 (`COGMASTER__EXPECT_FRAMES 1000000u`, `:6`). The new mode does not read an expect file, so the 64-record cap is moot; the equality test (D-02) uses one record, `boots@600`.

### Pitfall 9: `set -e` and the EXIT trap
`library.sh` runs with `set -eu`. With `trap 'rm -rf "$T"' EXIT` (the `scripts/firmware-window.sh:12` pattern) the trap also fires on a `set -e` abort, but not on SIGKILL (say so in `docs/testing.md`, D-11). `mktemp -d "$root/build/library/diag.XXXXXX"` works on BSD and GNU when the Xs end the template; create `build/library` first (`mkdir -p`) because on a fresh checkout it may not exist.

## Code Examples

### The guard (current, verbatim) and its replacement shape
```sh
# scripts/library.sh:11-13 today
if [ "${1-}" != run ]; then
  exec cmake --workflow --preset library
fi
# shape after: only run and diagnose skip the workflow
case ${1-} in run | diagnose) ;; *) exec cmake --workflow --preset library ;; esac
```
Keep any other argument, including none, on the old path.

### awk validator skeleton (portable dialect)
```sh
# Fields arrive as k=v tokens from library.cmake plus the runner line; nothing is echoed back unvalidated.
awk '
function num(s) { return s ~ /^[0-9][0-9]?[0-9]?[0-9]?$/ || s == "none" }
{ sub(/\r$/, "") }
$1 != "diagnose" || $2 != 1 { bad++; next }
# every field: closed pattern, or the whole record is malformed
...
END { printf "diag-other: malformed=%d\n", bad + 0 }
' "$tmp/records"
```
Print only with `printf "...%s...", validated` and never `$0`. Sort with `LC_ALL=C sort -k1,1nr -k2,2n -k3` for `diag-group` lines (count descending, then precedence, then kind code).

### Test scan shape (CMake script mode; no intervals, named failures)
```cmake
# Fault prevented: a byte, name, path or digest of an owned image reaching diagnose output.
function(diag_scan text)            # sets diag_scan_error to "" or "diagnose leak: <class>"
  ...
  string(FIND "${text}" "${name_canary}" at)   # string(FIND), not a regex, for literals and both slash forms
  if(NOT at EQUAL -1)
    set(diag_scan_error "diagnose leak: leak_name" PARENT_SCOPE)
  endif()
  string(REGEX MATCH "[^ -~\n]" bad "${text}")  # structure: only 0x20-0x7E and LF
endfunction()
```
Message text must match the pinned `REGEX` of `expect_fail.cmake`; that helper compares `rc` to the string `CODE` (`1` for `FATAL_ERROR`) and matches the pattern against combined stdout and stderr [VERIFIED: tests/cmake/expect_fail.cmake:1-12].

## Test design (answers item 3)

**Where tests live and how they register.** `tests/library/tests.cmake` is a dual-mode file: under `CMAKE_SCRIPT_MODE_FILE` it is the `library.synthetic` script, otherwise it registers inside `if(COGMASTER_FIXTURES)` with `cogmaster_add_test(NAME library.synthetic COMMAND ${CMAKE_COMMAND} -DROOT=... -DOUT=${PROJECT_BINARY_DIR}/tests/library -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DRUN=$<TARGET_FILE:cogmaster-run> "-DEMULATOR=${library_emulator}" -P ${CMAKE_CURRENT_LIST_FILE} LABELS library TIMEOUT 160)` [VERIFIED: tests/library/tests.cmake:287-298]. Register `library.diagnose` the same way beside it, pointing `-P` at `${CMAKE_CURRENT_LIST_DIR}/diagnose.cmake`, and pass `-DSEEDS=${PROJECT_BINARY_DIR}/tests/seeds` (built by `cogmaster-seeds ALL`, `tests/fuzz/tests.cmake:34-45`, available wherever `COGMASTER_FIXTURES` is on). `tests/library/diagnose.cmake` needs no row in `tests/policy/repository-checks.txt`: the unclassified-code glob covers only `tests/cmake/*`, `scripts/*`, `cmake/*`, `.githooks/*` and a few `tests/compat/` extensions [VERIFIED: tests/cmake/budgets.cmake:49-57].

**Inventory mechanics.** `cmake/CogmasterTests.cmake` sorts `name<TAB>labels` strings and requires equality with the file. Insert rows in byte order; the TAB (0x09) sorts before `.`, so `library.diagnose<TAB>library` precedes `library.diagnose.leak_title<TAB>library`, all before `library.synthetic<TAB>library` (ci.txt:303, wasm32.txt:305). Both files need every new name; labels are sorted and comma-joined (`portable,runner` for new runner tests). `quality.json` floors `tests.ci` (669) and `tests.wasm32` (627) are minimums; adding tests needs no edit [VERIFIED: tests/cmake/quality.cmake:77-79]. `ci.txt` currently has 671 data rows and `wasm32.txt` 629.

**Timeout comment.** The `# Timeout: N s; 2 x ...` convention is followed at registration sites, but `hygiene.timeout_comments` scans only `.github/workflows` `timeout-minutes` lines [VERIFIED: tests/hygiene/tests.cmake:517-519], so nothing mechanical checks a `tests/` comment. With no live run yet, write a provisional comment (the self-test fixture shows the word `provisional`), then re-derive from the first CI run per the twice-the-slowest rule.

**Owned fixtures that already cover the taxonomy** (measured with the existing runner and `--expect boots@600` on this commit):

| Fixture | Source | display / distinct / fault at 600 | Leaf it exercises |
|---------|--------|-----------------------------------|-------------------|
| halt image (`v` x 32768 plus canary tail) | built in the test | 0 / 1 / none | `dark_no_fault`/`never_draws` |
| `di-halt.sms`, `jr-self.sms` | `${SEEDS}` | 0 / 1 / none | same |
| `in-a-00.sms` | `${SEEDS}` | 0 / 1 / `unmapped_read`, dev `ctrl`, pc `$0002`, frame 0 | `fault_no_display`, class `port` |
| `ld-8000.sms` | `${SEEDS}` | 0 / 1 / `rom_write`, dev `cart`, addr `$8000` | `fault_no_display`, class `slot2` |
| `ed-00.sms` | `${SEEDS}` | 0 / 1 / `ed_undefined`, dev `cpu` | `fault_no_display`, class `cpu` |
| `static-display.sms` | `${SEEDS}` | 1 / 1 / none | `static_no_fault`/`frozen` (RAM still) |
| `mapper_korean.sms`, `mapper_hes_4pak.sms` | `${FIXTURES}` | 1 / 1 / none | `static_no_fault` |
| `boot_to_play.sms`, `vdp_lines_192.sms` | `${FIXTURES}` | 1 / 2 / none | pass (must not be printed) |

A new binary builder is unnecessary. Not covered by owned fixtures: `fault_alive`, `fault_static`, `latch_miss`, `late`, `animates_late`, `alive_still`. Cover their classification with a pure awk-level unit over hand-written runner lines (text only, one cheap test) rather than new ROMs. `CMake` cannot write NUL bytes with `file(WRITE)`, which is why the seeds come from `tests/fuzz/seeds.c`; do not try to hand-build images with zero bytes in the test.

**Cost control for negative controls.** Put the scan in `tests/library/diagnose.cmake` as a function. Then:
- `library.diagnose` (one real scenario of about 5 carts: halt+canary, `in-a-00.sms`, `static-display.sms`, `boot_to_play.sms`, one uncatalogued image) runs the scan over the real output (must pass) and asserts byte-equality of a second call, the retained file set `{diagnose.txt}`, and the ledger and page hashes before and after, with the existing `run` as the control that changes them.
- Four static controls (`leak_name`, `leak_path`, `leak_bytes`, `leak_digest`) run the same scan over a short literal output plus the planted leak in a separate `expect_fail` registration with `-DREGEX="diagnose leak: leak_name"` and so on: no runner invocation, sub-second.
- Two controls need a real path: `leak_title` (a one-cart directory whose catalog title is the name canary) and the hostile runner (one-cart directory plus the wrapper). Keep each to one 1800-frame cart, and let the scan assert both the title-subset failure and the name scan so the pinned class is stable.

That is about 8 emulations per leg for the whole test, down from about 30. Native cost on this host is about 0.7 s for a halt cart and about 1.5 s for a rendering cart at 1800 frames, so one native leg is roughly 10 s; ASAN and qemu-ppc lanes multiply this, and `COGMASTER_TIMEOUT_SCALE` multiplies the declared timeout (3 for `asan`, 8 for `be32`, 5 for `wasm32`) [VERIFIED: CMakePresets.json]. Start provisional (60 to 90 s) and tighten from the first CI durations (`scripts/ci-durations.sh`, rule 11).

**Runner tests** (`tests/runner/tests.cmake`, labels `portable,runner`): (a) usage cases for the `--diagnose` combinations via the existing `expect_exit(code regex args...)` helper; (b) on each fixture above, the printed `verdict600` equals the exit status of `--frames 600 --expect <boots@600 file>` (0 pass, 3 fail), the D-02 pin; (c) the printed line has the anchored closed shape (one regex) and holds no `observed_frame`, `op=` or path.

## Runtime and Cost (answers the bounded-time and Bash-limit questions)

- Per cart: probe plus one 1800-frame run, native Release on this host: 0.7 s (non-rendering) to 1.5 s (rendering) [VERIFIED: timed this session with the existing runner on owned fixtures]. A library of about 600 catalogued carts is therefore roughly 10 to 20 minutes natively plus process overhead; this exceeds the 10-minute foreground Bash limit, so use `run_in_background` for the real run, as D-23 says.
- Per-call wall-clock backstop: `library.cmake` uses `TIMEOUT 30` for 600-frame runs. Use at least `TIMEOUT 60` for the 1800-frame call so a loaded or slower host does not turn a slow run into `infrastructure_error`; any non-0, non-77 result is `infrastructure_error`, as today (`library.cmake:67-76`).
- `1800` frames, fixed and compile-time, bounds a hostile or jammed cart in emulated time (rule 4). The `--frames` option is a runtime value capped at 4,294,967,295; the diagnose mode has no such freedom.

## Docs, size budgets and release (answers items 3 and 5)

| File | Now (bytes) | Cap | Addition |
|------|-------------|-----|----------|
| `docs/runner.md` | 34,048 | 38,912 | option row, one section defining first non-blank and the line (about 1.5 KB) |
| `docs/conformance.md` | 34,733 | 38,912 | about 600 bytes beside the library-judge paragraph (`docs/conformance.md:3-15`) |
| `docs/testing.md` | 33,825 | 38,912 | command, `library.diagnose` entry, SIGKILL-skips-trap sentence (`docs/testing.md:11-14`) |
| `ASSET_POLICY.md` | 12,034 | none listed | three lines in section 4 (the local-only bullet that names `build/library/results/`) |

Budgets enforced by `tests/cmake/budgets.cmake`: `docs/*.md` below `quality.json` `docs.max_bytes` (38,912), README 12,288, `include/cogmaster.h` 24,576. Check-code: `check`-class lines sum to 887 and `src/` is 9,403 lines, so the strict one-tenth limit allows up to 940; `library.sh` and `library.cmake` are `operation` (uncounted) and `runner/` is outside `src/`, so this phase adds no counted lines [VERIFIED: computed this session].

Release: `scripts/check-title.sh` accepts `feat(runner): ...` (subject must start with a lowercase letter after `: `) and rejects `feat`/`fix`/`perf` with scope `ci|release|build|test|docs` and any CI-skip token in title or body [VERIFIED: scripts/check-title.sh:15-22]. `release-please-config.json` has `"bump-minor-pre-major": true`, so `feat` produces a minor bump. `scripts/classify-changes.sh` marks `.planning/`-only changes light and anything else heavy; none of `release.yml`, `ci.yml`, `publish.sh`, `release-please-config.json`, `cmake/packaging.cmake` changes, so `release_inputs=false` [VERIFIED: scripts/classify-changes.sh:1-45]. The five milestone commits sit on `milestone/v2.1-failing-carts` and are not on `origin/main` (`git log origin/main..HEAD` lists them), so decide with the owner's rule 10 mechanics whether phase 13 branches `phase/13-why-carts-fail` from `origin/main` or from the milestone branch; this is a planner or orchestrator choice, not a research finding.

## State of the Art

| Old approach | Current approach | When changed | Impact |
|--------------|------------------|--------------|--------|
| Judge `boots@600` only, read pass/fail | Per-cart facts plus a closed taxonomy | this phase | Causes become countable without reading dumps |
| Library run writes `.path`, `.probe`, `.error`, `.observation`, `.judge.txt` per candidate | Diagnose keeps one retained file | this phase | No per-image artefact to leak |

**Deprecated/outdated:** none relevant.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | mawk 1.3.4 builds from early 2020 lack regex interval expressions | Pitfall 2 | Low: the recommendation is to avoid intervals either way |
| A2 | `git rev-parse --short` length grows with repository size, so `core=` could reach 8 or more hex digits | Pitfall 3, R4 | Low: `cut -c1-7` is correct regardless |
| A3 | Windows runner stdout is CRLF, so the validator needs `sub(/\r$/,"")` | Pitfall 4 | Low: the strip is harmless on LF |
| A4 | A rendering cart is about 1.5 s per 1800 frames on the Linux and macOS CI legs, so the new test fits a provisional 60 to 90 s timeout | Test design | Medium: only the macOS arm64 dev host was measured; the first CI run settles it |
| A5 | A library of roughly 600 catalogued SMS/GG carts runs in 10 to 20 minutes natively | Runtime | Low: only decides `run_in_background`, which D-23 already prescribes |
| A6 | The phrase "the agent never reads a dump" is satisfied by awk-only output reading (no `cat` of scratch) | Pattern 4 | None for research; the plan restates D-12 |

All other claims in this document were read from files or run this session and carry a citation.

## Open Questions

All five are resolved (planning, 2026-11-12); each entry names the plan that implements the resolution.

1. **R1 wording and the PAL profile.** RESOLVED: include them. Plan 02 (13-02-PLAN.md, Task 1 step 3 and its R1 deviation) runs the first NTSC profile library.cmake derives, Japanese FM and Game Gear Japan rows included, and skips only the PAL profile. Original question: confirm the plan states "PAL profile skipped, NTSC default profile run" rather than "Japanese FM not run". If the owner truly wants Japanese carts excluded, 18 FM-profile fails and 2 GG Japan fails drop out of the denominators and `ledger_fail_not_in_run` rises by about 20. Recommendation: include them (they are default-profile rows).
2. **Fault-after-600 display (R3).** RESOLVED: both. Plan 01 (13-01-PLAN.md, R3 deviation, Task 2) prints the buckets 0, 1-9, 10-99, 100-599 and 600+ and computes F at frame 600 from `fault.frame < 600`; plan 02's classifier uses that F. Original recommendation: add a `600+` fault-frame bucket and compute F at frame 600 from `fault.frame < 600`; the grammar is Claude's discretion.
3. **`first_change` (R5).** RESOLVED: added. Plan 01 (R5 deviation, Task 1) prints `first_change=<n|none>`; plan 03 (13-03-PLAN.md, Task 2) builds the pass-row histogram from it. Original note: the pass-row histogram has no other source.
4. **Where `diagnose` writes `diagnose.txt`.** RESOLVED as recommended. Plan 02 (Task 1 step 1) reuses `COGMASTER_LIBRARY_OUT` for the retained-file directory in diagnose mode, default `build/library/diag`, under the existing inside-`build/` guard. Original question: D-11 names `build/library/diag/diagnose.txt`; the test needs an override inside its own build directory.
5. **Branch base for phase 13 (rule 10).** RESOLVED by the orchestrator: the phase branch is `phase/13-why-carts-fail`, cut from the current milestone-branch tip (milestone/v2.1-failing-carts), not from origin/main, so the five v2.1 milestone docs commits (ROADMAP, REQUIREMENTS, CONTEXT) travel with the phase; cutting from origin/main would strand them. This is a recorded deviation from rule 10's "off origin/main" wording, which the owner may override. Plan 01 (Task 1, first action) creates or selects the branch and records the deviation in its SUMMARY; plan 05 (13-05-PLAN.md, Task 3) requires that branch checked out and pushes and dispatches CI on it.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| `sh`, `awk`, `sort`, `sed`, `mktemp`, `dd` | `diagnose`, test wrappers | yes (macOS host) | BWK awk 20200816 | none needed; stay in the common dialect |
| cmake | script, presets, tests | yes | 4.4.3 (presets need 4.3, library alone 3.25 per `docs/testing.md:20`) | none |
| git | `core=`, tests | yes | present | none |
| built runner | probe and diagnose | yes (`build/library/cogmaster-run`, Release) | HEAD-adjacent build | `diagnose` rebuilds; tests use `COGMASTER_LIBRARY_RUN` |
| owned fixtures and seeds | tests | yes in `build/ci/fixtures` and `build/ci/tests/seeds` | built by CI | built by `cogmaster-seeds` and WLA-DX in the `ci` preset |
| `COGMASTER_ROM_DIR` | the real run | not available to this agent | n/a | owner exports it (D-22); plan checkpoint, exit 2 text on absence |

**Missing dependencies with no fallback:** the owner's library directory for the real run (by design). **With fallback:** none.

## Validation Architecture

`workflow.nyquist_validation` is `false` in `.planning/config.json`; this short map is optional and given for the planner's convenience.

| Requirement | Behaviour | Test | Command | File exists? |
|-------------|-----------|------|---------|--------------|
| DIAG-01 | line shape and fields; verdict equals `--expect boots@600` on fixtures; usage rules | `runner.diagnose.*` (portable, runner) | `ctest --preset ci -R runner.diagnose` | no, Wave 0 |
| DIAG-01 | library branch prints the closed grammar, fail rows only, pass rows only as histogram | `library.diagnose` | `ctest --preset ci -R library.diagnose` | no, Wave 0 |
| DIAG-02 | no byte, name, path, digest in any channel; retained set `{diagnose.txt}`; hostile runner leaves output unchanged except `malformed` | `library.diagnose` and `library.diagnose.leak_*`, `.hostile_runner` | same | no, Wave 0 |
| DIAG-03 | note exists, counts only, descending, `groups_total == fail_total` by exit status | plan acceptance commands (`test -s`, exit status of the command) | the real run, evidence per rule 11 | after the real run |
| Criterion 4 | ledger and page unchanged | in-test hash before and after, plus once on the real tree | `git diff --exit-code origin/main -- tests/compat/ledger.csv docs/compatibility.md` | exists (command) |

Sampling: per task `ctest --preset ci -R "runner.diagnose|library.diagnose"`; phase gate `cmake --workflow --preset ci`.

## Leak Surface (stands in for the ASVS table; `security_enforcement` is `false`)

| Channel | What could carry image-derived or owner-derived data | Control |
|---------|------------------------------------------------------|---------|
| runner stdout | `observed_frame` SHA-256, fault `op=$XX` (a ROM byte), probe digests and `title=` | `--diagnose` emits only its own closed line; validator allowlist |
| runner stderr | the file path (`probe.c:49,56`) | `ERROR_QUIET`, `2>/dev/null` |
| cmake script errors | script lines, values | per-image `cmake -P` output to `/dev/null`; no `.judge.txt` |
| scratch files | path, probe, observation | none written; `mktemp -d` with `umask 077` and an EXIT trap |
| catalog title | hostile bytes (RLO, ESC) | D-10 charset and 80-byte cap, `title=-` otherwise |
| directory listing order, dates, candidate index | fingerprint of the library | sorted by title with `LC_ALL=C`; no date; no index |
| the agent's own reads | `*.path`, `*.probe`, `*.error`, `*.judge.txt`, `*.observation` | D-12: read stdout or `diagnose.txt` only |

## Sources

### Primary (HIGH confidence, read this session)
- `runner/expect.c`, `runner/expect.h`, `runner/main.c`, `runner/args.c`, `runner/args.h`, `runner/probe.c`, `runner/probe.h`, `runner/exit_status.h`
- `include/cogmaster.h` (constants, `cogmaster_fault`, `cogmaster_cart_info`, `cogmaster_run_result`)
- `src/bus.c`, `src/cart.c`, `src/cartdb.c`, `src/cartdb.h`, `src/fault.c`, `src/frame.c`, `src/instance.c`, `src/mapper.c`, `src/mapper_ext.c`, `src/io.c`
- `scripts/library.sh`, `scripts/library.cmake`, `scripts/compat-common.sh`, `scripts/firmware-window.sh`, `scripts/check-title.sh`, `scripts/hygiene.sh`, `scripts/classify-changes.sh`
- `tests/library/tests.cmake`, `tests/runner/expect.cmake`, `tests/runner/tests.cmake`, `tests/cmake/expect_fail.cmake`, `tests/cmake/budgets.cmake`, `tests/cmake/repository.sh`, `tests/cmake/quality.cmake`, `tests/hygiene/tests.cmake`, `tests/policy/repository-checks.txt`, `tests/inventory/*.txt`, `tests/tests.cmake`, `cmake/CogmasterTests.cmake`, `tests/fuzz/tests.cmake`, `tests/fuzz/seeds.c` (header comment)
- `CMakeLists.txt`, `CMakePresets.json`, `quality.json`, `release-please-config.json`, `.github/workflows/ci.yml`
- `docs/runner.md`, `docs/conformance.md`, `docs/testing.md`, `ASSET_POLICY.md`, `.planning/REQUIREMENTS.md`, `.planning/STATE.md`, `.planning/seeds/SEED-016-failing-carts.md`, `.planning/config.json`
- `tests/compat/catalog.tsv` and `tests/compat/ledger.csv` (public tracked data; aggregate counts only)

### Local experiments (this session, no dump read)
- `cmake -P` regex interval probe (CMake 4.4.3): intervals unsupported.
- Existing runner on owned fixtures and seeds: `--expect boots@600` facts table; `--frames 1800 --hash` timing.

### Secondary / Tertiary
- None. No web source was needed; no external package is involved.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - no new dependency; components named by file and line.
- Architecture (D-04, sharing the verdict, branch points): HIGH - read from source and the guard text quoted verbatim.
- Pitfalls: HIGH for repository tripwires and CMake regex; MEDIUM for awk portability beyond this host (A1) and Windows CRLF (A3).
- Cost and timeout estimates: MEDIUM - one host.

**Research date:** 2026-11-12
**Valid until:** 2026-12-12 (stable; invalidated by any change to `runner/expect.c`, `scripts/library.*`, or the inventory files)
