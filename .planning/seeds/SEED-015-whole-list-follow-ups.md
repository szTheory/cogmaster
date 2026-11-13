---
id: SEED-015
status: dormant
planted: 2026-11-12
planted_during: phase 12, plan 09
trigger_when: the next ledger work, the next CI-speed work, a pin bump that changes a catalog count, or a reviewer reopens one of the items below
scope: medium
audit_acknowledged:
  milestone: v2.0
  at: 2026-11-12
  status: dormant
---

# SEED-015: whole-list ledger and CI-speed follow-ups left out of the phase

## Why This Matters

The phase gave every Master System and Game Gear catalog identity one default-profile row, a closed reason for every
`unsupported` and `skipped` row, a bounded page and an exit command tied to the libretro flag. Each item below was left
out on purpose: no evidence settles it, it needs a measurement only the owner's hardware can make, or it edits the
release path, and `ci.yml` is edited at most once per phase, in the last push. None of them changes a hash or a row
today. Each says why it is not in the phase and what evidence would decide it.

## Ledger rows

- **PAL and Japanese FM `unknown` rows for every Master System identity.** About 2,100 rows would fit the 8,192 row
  cap, but nothing is behind them: a row with no run says only what the default row already says. The owner's library
  run adds the profiles a real cart reaches. Decided by: a reason to list a profile no run has judged.
- **A catalog `video` column for the 50 Game Gear entries MAME runs in Master System mode.** They are seeded with the
  Game Gear video and may judge `fail`, a profile fact. Needs a measurement of what a real Game Gear shows in Master
  System mode; until then the catalog stays without the column.
- **Real rows for the two `baddump` entries and the three short dumps.** They carry a derived id and a `skipped` reason.
  A real row needs a padding rule, because the loader fills with `0xFF` and a padded run would claim a result for bytes
  nobody dumped. Decided by: a source that says what the missing bytes are.
- **Profile rows from the 16 MAME `usage` hints.** A hint names a controller or mode a title needs. Turning a hint into
  a profile row needs the hint to be trusted per title; decided by the owner's run showing which hints a real cart
  confirms.
- **Per-alias `supported`.** `supported` is read per record but a shared image keeps the worst verdict, so the Game
  Gear twin that MAME marks working is judged `skipped` because its Master System twin is not. Per-alias support would
  let the twin be judged. Decided by: a pinned example where the verdicts differ and the twin runs.
- **A reason for `partial` entries.** Three Game Gear entries are `partial`, runnable and `unknown`. A reason needs
  evidence of what is partial; MAME's attribute does not say.
- **A promotion path for `skipped` rows other than a pin bump.** Today a row leaves `skipped` only when the pin stops
  saying so. A second path (a measured pass on an owned dump) would let the library judge lift one; it conflicts with
  the rule that a status never moves without evidence in the catalog, so it needs its own design.
- **Mark III, PAL-M and PAL-N.** Machine and timing profiles that no row can name yet; they wait on the measured
  accuracy work.
- **Hash-less audio.** An audio predicate that needs no committed digest of our output stays in its own seed.

## Procedures that stay in their own seeds

- The pad and device measurements and the BIOS follow-ups stay in the seeds that hold their commands; the owner runs
  them in the same sitting as the library run when convenient.
- A `scan:` counts section for the library run stays in the threshold seed beyond its counts line.

## Nightly freshness

A nightly step that fetches the three lists and compares the catalog, the pin counts and the cartridge table with the
committed files would catch a hand edit of `skipped` status, which only the pin counts tie today. It edits `ci.yml`, so
it waits for a phase that does not already edit it. Decided by: a recurring hand edit, or a pin bump that went stale.

## CI items

Measured on the phase's hosted run (dispatch of `ci.yml` on the phase branch, run 43153144670, head with the whole-list
ledger): build job on windows-arm64 214 s, be32 293 s, wasm32 55 s, the hygiene lane 38 to 45 s. The build timeout
widened from 6 to 9 minutes by the mechanical rule (2 x 252 s from the later dispatch, run 43158408243; 214 s was the
first sample; `sh scripts/ci-durations.sh ci.yml 40`); that was the only `ci.yml` line the phase edited. Earlier measurements of pull request runs: 627 to 637 s with the release
rehearsal, 302 to 321 s without; be32 and windows-arm64 are the critical path.

The owner's standing instruction: whenever CI runs or finishes, evaluate the low-hanging fruit to make it faster
without hurting quality. Read `sh scripts/ci-durations.sh tests <run-id>` and the job seconds, record any test above
half its timeout, and write the finding here or in a phase note, never only in a chat.

- **The `docs/maintaining.md` "CI speed" rewrite**, with three rules: a release-input edit doubles the run (the
  rehearsal is about six extra minutes, so a pull request that edits `ci.yml`, a release script or a release config
  pays it once more per push); open the pull request once, at the end, because every push to an open pull request
  reruns the matrix; a dispatch compares the ledger with itself (the protected comparison takes its base from the
  dispatched commit), so the base-judge property is proven locally against `origin/main` and recorded as a command and
  an exit status.
- **The duplicate attestation in the publish verify step** (about 120 s of release time). Edits the release path, so it
  waits for a phase that edits `ci.yml` anyway.
- **The dead branch from the first to the second ledger format** in the protected comparison step, made dead by the
  format 2 migration. Removing it edits `ci.yml`; same trigger.
- **Phase 15 timeouts findings** (`.planning/phases/15-re-run-merge-and-timeouts/15-RUN-EVIDENCE.md`). Still open: the
  per-test ctest `TIMEOUT` values of `library.synthetic` (160 s), `compat.ratchet` (142 s), `compat.public_update` (104 s) and
  `compat.csv` (45 s; the Phase 12 review leftover below names the rule to settle them), because `hygiene.timeout_comments`
  scans `timeout-minutes` lines only and the roadmap named jobs, not tests. `runner.diagnose` (about 311 s on the be32 lane at
  the Phase 15 discuss read) and `library.diagnose` (about 142 s) arrived in Phases 13 and 14 and roughly doubled the be32
  lane, so a cheaper be32 lane is a seed to judge against the be32 coverage it should keep; making a job faster inside the
  timeouts pull request would have put its timeout out of step with the runs it cites. The three rules of the "CI speed"
  rewrite bullet above are now one paragraph in `docs/maintaining.md`, while that section's critical-path sentence (macOS
  builds, then the rehearsal) is still the stale line the milestone audit named. The jobs left with stale run IDs (wasm32,
  retroarch, perf, hygiene, nofp) satisfy the rule against the fresh read and were not edited.
- **Rejected with data, do not reopen without new numbers:** a cost ordering of the be32 tests, dropping the libretro
  host tests, caching, and path-filtering the `edited` filter. The phase's CI review measured each and found no saving
  that kept the checks it removes.

## Where it links

Plan 06 of the phase holds the library judge and its hand-off in SEED-013; SEED-009, SEED-010, SEED-011 and SEED-012
hold the scan, pad, BIOS and audio procedures named above; SEED-003 holds the machine and timing profiles.

## Phase 12 review leftovers

Left out of review fix round 1 on purpose; each is a seed, none moves a hash today. The round fixed the rest of both reviews;
`12-REVIEW-FIX.md` lists what and by which commit.

Ledger review:

- **Base-copy hardening of the protected comparison** (the part of the trust gap the round did not close). The base judge
  reads `tests/compat/catalog.tsv` and `reasons.txt` from the head checkout, so a pull request that also edits those can
  release its own `unsupported` and `skipped` rows. The round refuses an empty or statusless catalog and documents the trust
  model (the comparison guards against accidental or unreviewed flips, not against an author who can edit the catalog and
  `ci.yml`). The hardening: the `ci.yml` step that copies the baseline closure also copies those two files from the base into
  the copied judge's tree and runs it with `COMPAT_CATALOG` and `COMPAT_REASONS` pointing there for R1, uses the head catalog
  only for the "board arrived" half of R3, and requires the head `src/cartdb.inc` fact. It edits `ci.yml`, so it waits for a
  phase that edits it anyway. Decided by: a pull request that moves a status without its board.
- **Settle the sample-based timeouts again after the ship run**: `library.synthetic` (160 s from one sample), `compat.ratchet`
  (142 s), `compat.public_update` (104 s) and `compat.csv` (45 s) are exactly twice one or two windows-arm64 samples. After
  the pull request run adds samples, set each to the larger of twice the slowest and the slowest plus 60 s with the run ids.
  Cheaper: `library.synthetic` runs about eight `library()` calls and each now spawns one `cmake -P exit.cmake`.
- `.gitattributes` marks `tests/compat/ledger.csv` `linguist-generated`, which collapses its pull request diff; keep it for the
  catalog and `src/cartdb.inc`, drop it for the ledger or say so in `docs/conformance.md`.
- `md()` escapes `& < > |` and the markdown set but not GFM bare autolinks (`www.`, `https:`, an address with `@`); entity-encode
  `:` and `@` or print titles in a code span. Today's 1,444 titles are clean.
- `compat-update.sh` honours `COMPAT_CATALOG` and `COMPAT_LEDGER` from the environment (a test seam in a production script);
  print the catalog path to stderr when it is not the tracked one, or read the seams only under a test variable.
- The `exit:` block of `library.sh run` reads the pre-merge ledger when the merge is held; read `next-ledger.csv` then, or say so.
  Its `exit: failed` line is cut from CMake's wrapped message.
- Cosmetics of the library judge and the parser: the `pal:` and `fm:` counts include runs for ids whose facts `update` discards
  (say "runs"), a `reasons.txt` line with a trailing comment parses in the awk but not in `exit.cmake`, and the 40-line cap on
  `reasons.txt` is enforced nowhere.

Core review:

- **The generator rescans a record quadratically**: a record with tens of thousands of empty `rom` data areas takes about 24 s at
  2.4 MB (the bounded and digest-pinned input makes this a robustness item, not an exposure). Bound the inner `<rom ` search
  to the data area (`memchr`-limited, or end the area with a NUL while scanning) and add a hostile record with a timeout to
  `tests/cart/catalog.cmake`.
- The `exit 77` runner assertions for `gg-8k.gg` and `di-halt.sms` repeat earlier cases and reach the size heuristic, not the
  Graphic Board table path (necessity); drop them or reword their comment. The tab, CR and LF guard in `write_catalog` cannot
  fire because `check_title` rejects every byte below 0x20 (necessity); remove it or comment it as a second defence.
- The entry reconciliation in `tools/cartdb.c` holds by construction (each record yields one fact), so it checks a coding
  slip, not data; call it a self-check, or have `write_counts` read the previous counts file and fail on a difference unless
  `--update` is given.
- A future 8 KiB Game Gear image stops the generator with "board 0 and no slot" and no hint; say "add a slot to the table in
  `tools/cartdb.c`", and mark `fail` noreturn behind a portable macro.
- `docs/conformance.md` lists the catalog status values without `excluded` and the board value `compound`, and `docs/boards.md`
  explains the Graphic Board row by what it once read as, which belongs only with the behaviour revision in `docs/embedding.md`.

## Milestone audit leftovers

Found by the v2.0 milestone audit on the tree of release v0.12.0; none is a blocker and none moves a hash. Append-only
record for the next tune-up or ledger work (`.planning/v2.0-MILESTONE-AUDIT.md`).

- **Job timeouts drifted from the twice-the-slowest rule** (`sh scripts/ci-durations.sh ci.yml` and `nightly.yml`,
  2026-11-12). `ci.yml` asan: slowest 153 s, so 2 x 153 s needs 6 minutes against the 5 it keeps (comment `2 x 84 s, 37
  runs`). `nightly.yml` bench: 10 minutes against 2 x 368 s; `bench-wasm32`: 15 minutes against 2 x 456 s; the `fuzz`
  comment says 90 s against a live 144 s (the 5-minute floor still holds). `digest.yml` still says "no runs yet" though a
  digest run exists (55 s). `hygiene.timeout_comments` checks only the comment format and `timeout >= 2 x S` for the S the
  comment states, so CI cannot see the drift; the weekly digest names asan, bench and bench-wasm32 at its first scheduled
  run. Decided by: the next tune-up (re-measure, edit the workflows once). **Done in Phase 15** (table and run IDs in
  `.planning/phases/15-re-run-merge-and-timeouts/15-RUN-EVIDENCE.md`): asan 5 to 9 minutes, bench 10 to 13, bench-wasm32
  15 to 16, the fuzz and digest comments re-cited, all in one commit. The same read showed `build` (9 minutes against 2 x 332 s)
  and `be32` (15 against 2 x 609 s) below the rule too, so both were corrected in that commit (build 12, be32 21); the cause is
  test growth, not the runner.
- **50 Game Gear rows are judged as `video=gg` though MAME runs them in Master System mode.** The core resolves a Game
  Gear's video from the table (`src/cartdb.inc` holds 50 such rows), but `scripts/compat-seed.sh` writes `cfg:video=gg` for
  every `gg` row and `scripts/library.cmake` always runs `--video-mode gg`; a `pass` or `fail` is then a profile fact, and
  no user doc or test says so. Same trigger as the catalog `video` column item above.
- **The 188-in-1 row is `skipped` with the reason `mame_no`, and the runner still exits 77 for it.** MAME marks it
  `supported="no"` and the whole-list rule order puts that first; the Master Gear row is `skipped` too, so the reason
  tokens `korean_188in1` and `mgear` are unreachable until a pin bump. The slot table still refuses both. The Korean-board
  requirement text names 188-in-1 as `unsupported` and was not amended.
- **Libretro `cogmaster_region = pal` with Game Gear content fails the load** (`libretro/libretro.c` sets PAL whatever the
  machine and the core rejects PAL on a Game Gear), while the port and FM options clamp. The runner's equivalent exits 77
  by design. Decide: clamp to NTSC and say so in the option text, or keep the refusal and document it; add a Game Gear
  case to `tests/libretro/presentation.h`.
- **The default FM rule is a literal in five places** (`src/instance.c`, `scripts/compat-common.sh` twice,
  `scripts/compat-seed.sh`, `scripts/library.cmake`). They agree today. Flipping every `fm=on` row to `fm=off` leaves
  `compat.whole_list.runnable` green; only `compat.whole_list.fresh` fails, so a rule drift regenerated consistently is
  not caught. Add a case that fails `compat.whole_list.runnable` when the rule flips, or name the rule once.
- **The `policy.claims_scoped` flag-absent control tree is thinner than the real tree.** It holds only the README, the
  page, the saves page and the `.info`, so the `docs/*.md` loop and the page's second "Experimental" sentence are vacuous
  there. SEED-013 now lists `docs/testing.md` and that sentence; a control over the real docs would make the proof honest.
- **Smaller drift:** `docs/maintaining.md` "CI speed" names the macOS builds and the rehearsal as the critical path while
  be32 (326 s) and windows-arm64 (253 s) are; `docs/conformance.md` says "two fixtures" and "two fuzz entries" (nine rows
  over four fixtures; six fuzz targets) and `CMakePresets.json` says "only the two fuzz targets"; `docs/testing.md` lists
  four fuzz targets and omits eeprom and pad; `ASSET_POLICY.md` says compat-update projects only existing ledger cases and
  that local runs read the BIOS folder "recognised by digest" (only the window scan does); `tests/perf/allow.txt` names
  phases in a file that outlived them; the quality floors (669, 627) sit 2 below the inventories.
