# Phase 13: Why carts fail - Context

**Gathered:** 2026-11-12
**Status:** Ready for planning

<domain>
## Phase Boundary

Give the failing default-profile rows a known cause, grouped by size, from a diagnostic over the owner's library that
leaks nothing from the dumps. The phase delivers a read-only diagnostic (a runner mode plus `library.sh diagnose`), a
synthetic-fixture leak test with a negative control, and a counts-only planning note of cause groups. No core fix is
made and no ledger or page row moves (DIAG-01, DIAG-02, DIAG-03).

Fixing causes is Phase 14; merging flipped rows and re-deriving CI timeouts is Phase 15. This phase makes no numeric
target for flips and does not touch `ci.yml`.

</domain>

<decisions>
## Implementation Decisions

Method: the owner's standing fan-out boilerplate was applied. Four advisor agents (plumbing, leak contract, cause
taxonomy, run protocol and release mechanics) each read the code, applied the role lenses and ran a self-adversarial
pass; the orchestrator then reconciled them and ran a cross-cutting adversarial pass. All areas were selected; the
owner's boilerplate asks for one coherent set of recommendations that is followed, so no per-area vote was held. The
owner's earlier decision stands: the agent runs the diagnostic in a shell where the owner passes `COGMASTER_ROM_DIR`
(REQUIREMENTS, scope decisions of 2026-11-12). Do not reopen it.

### Diagnostic surface

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

### Leak-proof output

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

### Cause groups and the note

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

### Run protocol, docs and release

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

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and requirements
- `.planning/ROADMAP.md` (Phase 13) — goal, success criteria, the dump-handling note.
- `.planning/REQUIREMENTS.md` — DIAG-01 to DIAG-03; the scope decisions of 2026-11-12 (agent runs the diagnostic, no numeric target).
- `.planning/seeds/SEED-016-failing-carts.md` — the ledger evidence by mapper and the open question this phase settles.
- `.planning/seeds/SEED-013-pal-fm-library-run.md`, `.planning/seeds/SEED-015-whole-list-follow-ups.md` — how the earlier library run was done.
- `.planning/LESSONS.md` — XFER-044 (tooling bloat), XFER-045 (invented ceremony), XFER-054 (evidence from the command that ran).
- `CLAUDE.md` — rules 3, 5, 6, 7, 8, 9, 10, 11, 12.

### The judge and the check
- `scripts/library.sh`, `scripts/library.cmake` — the run mode to branch from; its merge block must stay unreachable from `diagnose`.
- `docs/runner.md` ("Accepted expectations", `boots@F`), `runner/expect.c`, `runner/main.c` (`print_fault`), `runner/probe.c` — the predicate and the fault line.
- `docs/conformance.md` — library-judge text and the ledger grammar.
- `tests/compat/catalog.tsv`, `tests/compat/ledger.csv` — public catalog titles and rows (read-only here).
- `include/cogmaster.h`, `docs/boards.md`, `src/bus.c`, `src/mapper.c`, `src/mapper_ext.c`, `src/cart.c` — fault record, boards and mapper choice.

### Tests and policy
- `tests/library/tests.cmake` (`library.synthetic`: stand-in catalog, ledger and page, planted names), `tests/cmake/expect_fail.cmake`, `scripts/firmware-window.sh` (trap and scratch pattern), `tests/inventory/ci.txt`, `tests/inventory/wasm32.txt`.
- `tests/policy/repository-checks.txt`, `tests/cmake/budgets.cmake`, `quality.json` — check-code classification and doc budgets.
- `ASSET_POLICY.md` sections 1 and 4 — what may be published about owned dumps.
- `scripts/check-title.sh`, `release-please-config.json`, `.planning/preparation/DECISIONS.md` DEC.11, DEC.18, `.planning/preparation/ENGINEERING.md` section 5 — release rules.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `library.cmake` probe step (`--probe`, identity SHA-1, `mapper_guess`, `board_source`) and its catalog lookup by 40-hex key.
- `library.synthetic` stand-in catalog, ledger, page and planted home-path and e-mail names; the `COGMASTER_LIBRARY_RUN` and `COGMASTER_LIBRARY_EMULATOR` wrapper seam (`flaky-runner.sh`).
- `expect_fail.cmake` negative controls (`fixtures.boot_to_play.wrong_hash`) and `runner.bios.no_path_echo`.
- `print_fault` kind and device tables and `cogmaster__board_name` for closed-vocabulary printing.
- The held-merge assertion in `tests/library/tests.cmake` (around lines 446-450) as the model for the ledger-unchanged check.

### Established Patterns
- Operation-class scripts do not count against the one-tenth check budget; new script files need a row in `tests/policy/repository-checks.txt`, so extend `library.sh` and `library.cmake`.
- Every printed value is a number, an enum name or a catalog title; titles are hardened (phase 12) before they reach a page.
- Every gate names the fault it prevents; negative controls assert a specific failure, not any failure.
- The core is deterministic and clock-free, so counts are a function of core revision plus library.

### Integration Points
- `library.sh` argument guard (line 11) and the point before the merge block; `runner/main.c` and `runner/args.c` for the mode; `runner/expect.c` for the shared 600-frame predicate.
- Footguns found in the code: the raw `boots` observation line carries `observed_frame=<sha256>` and the fault line carries `op=$XX` (a ROM byte), so neither may be forwarded; `--expect` caps at 64 records; the ledger loop also runs PAL and FM rows once they exist, which would diagnose a cart twice if reused; the diagnostic derives "fail" from a fresh run, which matches the ledger only while the core is unchanged since the library run (v0.13.0), hence the revision in the output and the totals block.

</code_context>

<specifics>
## Specific Ideas

- The pass-row baseline (first-moving frame per mapper) answers FIX-02 directly: passes clustering near frame 600 point at the cap; passes moving by frame 60 point at a sound check.
- Resolve the roadmap's "frames to first non-blank sample" once, in `docs/runner.md`, so the diagnostic, the note and Phase 14's fixture all mean the same thing.

</specifics>

<deferred>
## Deferred Ideas

- A Start-press scripted rerun to separate "waiting on input" from "frozen": Phase 14, only for the group that needs it.
- Per-instruction trace or CPU register output: Phase 14 uses `--trace` on one exemplar.
- A PAL or Japanese FM diagnostic: no value (PAL fails are NTSC fails).
- A CI job or counted "ledger and page unchanged" script, a library fingerprint, a second output format, attempt quotas: rejected as overbuilding (rule 7, rule 8).

</deferred>

---

*Phase: 13-why-carts-fail*
*Context gathered: 2026-11-12*
