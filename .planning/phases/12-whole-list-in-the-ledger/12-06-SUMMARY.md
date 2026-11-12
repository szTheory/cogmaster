---
phase: 12-whole-list-in-the-ledger
plan: "06"
subsystem: library-judge
tags:
  - library
  - ledger
  - adoption
  - seed
status: complete

requires:
  - 12-05
provides:
  - scripts/library.cmake judges by profile, not by id, so a seeded default row flips and a missing PAL or Japanese FM profile is still added on a whole-list ledger
  - library.synthetic over a stand-in ledger written by the seed script, with the flips, the one PAL row, a skipped control, a device control and a cart judged by its own ledger row
  - SEED-013 with the owner's one batched own-library run and the removal pull request text for the experimental flag
affects:
  - 12-07
  - 12-09
  - 12-10

actuals:
  tokens: 5300
  tasks: 3
  commits: 3

plan_head_before: ea47145f4b455ce1ecfaedf32909a3460e36de8e
plan_head_after: 9ec94e39e19337f913d8ea81eafea3063ca6f38e
commits: 3

tech-stack:
  added: []
  patterns:
    - "A derived profile is skipped only when the loop over ledger rows ran it for this image; a probe identity that is not the ledger id (a synthetic stand-in) leaves every derived profile to the derived part, which flips the seeded row through the key that already exists"
    - "A wrapper that rewrites the probe identity and drops the fixture hashes gives an owned image a catalog identity, so the real-cart path is exercised without a ROM"

key-files:
  created: []
  modified:
    - scripts/library.cmake
    - tests/library/tests.cmake
    - .planning/seeds/SEED-013-pal-fm-library-run.md
    - .planning/seeds/SEED-009-scan-thresholds.md

key-decisions:
  - "listed means the loop over ledger rows ran the profile for this image, not that the ledger holds a row of the id"
  - "A skipped id still gets its derived runs and the update guard discards the facts; a device id (a non-pad port among its ledger rows) gets none"
  - "The cart judged by its own ledger row is proven with a probe wrapper rather than left to the unit of reasoning, because the synthetic images never carry a catalog identity"

requirements-completed:
  - LEDG-02

coverage:
  - id: C1
    description: "On a whole-list stand-in ledger the library run flips the seeded default rows of an export-header image, a Japan-header image and a Game Gear image to pass or fail with core= evidence, adds exactly one export PAL row, and leaves no unknown row beside a judged one"
    requirement: LEDG-05
    verification:
      - kind: integration
        ref: "library.synthetic: rows_added=1 ratchet=ok page_bytes=3032, three flips and the PAL row asserted by exact key; negative control: the derived part treating any row of the id as judged turns it red (exit 8), restored green"
        status: pass
    human_judgment: false
  - id: C2
    description: "A skipped id keeps its status and reason evidence whatever the run observes: its derived runs are produced and the update guard discards the facts, and the ratchet is a second refusal"
    requirement: LEDG-02
    verification:
      - kind: integration
        ref: "library.synthetic skipped control; negative control: the update guard dropped from compat-common.sh turns library.synthetic red (exit 8, the merge is refused), restored green"
        status: pass
    human_judgment: false
  - id: C3
    description: "A catalog id whose ledger rows hold a non-pad port keeps its one unknown row and is given no pad run and no pad row"
    requirement: LEDG-02
    verification:
      - kind: integration
        ref: "library.synthetic device control; negative control: the non-pad rule ignored turns it red (exit 8), restored green"
        status: pass
    human_judgment: false
  - id: C4
    description: "A cart whose probe names its ledger id has its default row judged once by the loop over ledger rows and only its missing PAL profile derived, so the NTSC profile is not run twice and the PAL row is still added"
    requirement: LEDG-05
    verification:
      - kind: integration
        ref: "library.synthetic known-identity case: pal line carts=0 ran_pal=1, rows_added=1; negative control: the ran-profile record removed turns it red (exit 8), restored green"
        status: pass
    human_judgment: false
  - id: C5
    description: "Planted home-path and address-shaped file names leave no trace in public-facts.csv, results.csv, standard output, the merged ledger or the page, the results directory is ignored by git, and a docs cap of 100 holds the merge with both stand-in files byte-identical"
    requirement: LEDG-02
    verification:
      - kind: integration
        ref: "library.synthetic privacy loop over the two added images, git check-ignore build/library/results/1.path exit 0, held-merge case unchanged"
        status: pass
    human_judgment: false
  - id: C6
    description: "The library preset configures with an empty library directory and refuses a missing one with its documented message; no file under build/library/results was read"
    requirement: LEDG-02
    verification:
      - kind: integration
        ref: "COGMASTER_ROM_DIR=<empty temporary directory> cmake --preset library exit 0; a missing directory exits 1 with COGMASTER_ROM_DIR must name an existing directory"
        status: pass
    human_judgment: false
  - id: C7
    description: "The owner's own-library run and the removal of is_experimental in the pull request that merges its rows are written down as one batched hand-off after the release, never a gate or a user acceptance step; the SEED-009 counts and the SEED-010 and SEED-011 commands stay in their seeds"
    requirement: LEDG-05
    verification:
      - kind: manual_procedural
        ref: .planning/seeds/SEED-013-pal-fm-library-run.md
        status: not_run
    human_judgment: true
---

# Phase 12 Plan 06: Library judge by profile Summary

The library judge now adopts by (id, profile): on a whole-list ledger a seeded `unknown` default row flips to `pass` or `fail` through the existing update path and a missing PAL or Japanese FM profile is still added, a `skipped` fact is discarded and a device title gets no pad row, proven on synthetic images with controls that fail without each rule; SEED-013 carries the owner's one batched run and the flag-removal text.

## Performance

- **Tasks:** 3 of 3 (task 1 is the tracer: the profile rule and the seeded stand-in end to end)
- **Commits:** `6bdf8a9` (task 1), `e0a1374` (task 2), `9ec94e3` (task 3)
- **Counted check code:** 879, unchanged (plan cap 897; `library.cmake` is operation, `tests/library/tests.cmake` is behavior, the seeds are data). The src figure printed is 9,403 lines, one above the 9,402 of the context, not from this plan.
- **Docs bytes:** no file under `docs/` or the README was touched (`git diff --stat ea47145..HEAD -- docs` is empty); SEED-013 is 4,786 bytes and SEED-009 4,897.
- **Ledger and page:** `git diff --quiet ea47145 -- tests/compat/ledger.csv docs/compatibility.md` exit 0 (plan 5's final commit, `ea47145`, is the base).

## Accomplishments

1. **Profile-level adoption (task 1).** `scripts/library.cmake` records, in the loop over ledger rows of the probe's id, each pad-default `boots@600` row it ran as a `machine|region|tv|fm` token. The derived part skips a profile only when its token was run for this image and the judged id equals the loop's id (a real cart, whose probe identity is its ledger id). A probe identity that is empty or a fixture name (every synthetic stand-in) leaves the list empty, so the derived part runs the profile and the update guard's key flips the seeded row. An id with any non-pad port among its ledger rows gets no derived profile. The first loop, the unsupported and malformed paths and the content of the public-facts rows are unchanged.
2. **Seeded stand-in (task 1).** `library.synthetic` writes a 10-field temporary catalog, seeds the fixture rows with `scripts/compat-seed.sh` and a fixed date, and builds the page from the result, so the stand-in is a whole-list ledger in miniature. Expected: the three default rows flip with `core=` evidence, one PAL row is added (`rows_added=1`, new lines counted; it was 4), no unknown row stays beside a judged key.
3. **Skipped and device controls (task 2).** The plain image is the `skipped` id (`mame_no`) and a second owned image the device id (`port1 lphaser`), both named with planted path and address strings. The skipped row keeps `skipped` and `reason=mame_no` while its two derived facts appear in `public-facts.csv` (so the guard, not an absent run, is what keeps it); the device id has one `unknown` row and no fact at all.
4. **Cart judged by its ledger row (extra case).** A probe wrapper rewrites the identity to the catalog SHA-1 and drops the fixture hashes, so the real-cart branch runs without a ROM: the default row is judged by the loop, the NTSC profile is not run again (`pal: carts=0 ran_pal=1`) and the PAL row arrives. Without this the `ran` list would never be non-empty in CI.
5. **Seeds (task 3).** SEED-013: "Why the merge can go through now" (`merge: held` is a runaway guard), "The flag leaves with the rows" (the removal pull request: `is_experimental`, "Experimental. " in the `.info` description and the page intro, the README Status paragraph, the experimental sentences in three docs; the claims-check exemption is not edited; `unmet` keeps the flag; a red exit command is the tie or triage, never a hold), "What stays elsewhere" (SEED-010, SEED-011, the SEED-009 scan section). SEED-009 gains a note that no `scan:` section is printed in this milestone.

## Evidence (command, exit status; local, repository root)

- `cmake --preset ci > /dev/null && cmake --build --preset ci && ctest --test-dir build/ci -R '^(library|compat\.public_update|compat\.ratchet)' --output-on-failure` exit 0 (3 tests passed) after task 1.
- `ctest --test-dir build/ci -R '^library\.synthetic$' -V` exit 0; log lines `merge: rows_added=1 ratchet=ok page_bytes=3032`, `pal: carts=2 ran_pal=2 boots_pass=2 boots_fail=0`, `fm: carts=1 boots_pass=0 boots_fail=1`, `library-judge 1`. Both derived runs of the skipped image returned a verdict, so the pal line is `carts=2 ran_pal=2` with `boots_pass=2`.
- `cmake --workflow --preset ci` exit 0 (after task 2); `cmake --workflow --preset hygiene` exit 0 (after tasks 1, 2 and 3).
- `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0, `879 check lines / 9403 src lines, strict one tenth`.
- Task 3 verify: `test -f ... && grep -q is_experimental ... && ! grep -Eq '[0-9a-f]{40}' ...` exit 0; `grep -c 'Why the real merge waits'` prints 0; `grep -q judged_cart_floors`, `grep -qi gate` (SEED-013) and `grep -q scan` (SEED-009) exit 0; `git diff -U0 origin/main -- .planning/seeds` exit 0 and the added lines hold no `/Users/` or address.
- `COGMASTER_ROM_DIR=<empty temporary directory> cmake --preset library -B <scratch>` exit 0; with a missing directory exit 1 and the message `library: COGMASTER_ROM_DIR must name an existing directory`. No ROM directory of the owner was named, `.env.local` and `build/library/results/*.path|.probe|.error` were not read.
- Awk portability (the stand-in flow: seed over the fixture rows and the 10-field catalog, `update` with a flip, a discarded skipped fact and a PAL row, `docs` over the result): output `cmp`-equal under macOS awk, mawk 1.3.4 (`docker run ubuntu:26.04`) and gawk 5.3.2 (installed in the throwaway container only), exit 0 each. No awk code changed in this plan; the check covers the new inputs.
- `gcc:13` container check: no C file was touched; not applicable.
- No hosted run in this plan (D2 is plan 10); no run URL.

## Negative controls (each: python edit, `git diff --stat`, `ctest -R '^library\.synthetic$'`, restore, rerun)

| Control | Mutation | Result |
|---|---|---|
| Any row of the id means judged | `nonpad` set for any ledger row of the judged id (the old `listed`) | library.synthetic exit 8: `must add the PAL row`, `rows_added=0`; restored exit 0 |
| Ran-profile record | the `list(APPEND ran ...)` removed | library.synthetic exit 8 at the known-identity case (NTSC rerun, `carts=1`); restored exit 0 |
| Update guard | `cs[f[1]]!="unknown"` removed from `compat-common.sh` | library.synthetic exit 8: the skipped fact became a pass and the ratchet (a head pass for a catalog skipped id) refused the merge, so the run exited non-zero before the control's own assertion; restored exit 0 |
| Non-pad rule | the port condition made `AND FALSE` | library.synthetic exit 8: the device image got a pad row (`rows_added` 2); restored exit 0 |

The guard control is caught by the ratchet's rule as well as by the update guard, so the test is red either way; the assertion on the merged skipped row only fires when the ratchet is also bypassed. This is defence in depth, not a gap.

## Cost and timing (measured, no claim about hosted time)

- `library.synthetic` locally: 5.48 s before this plan (5.73 s with the seeded stand-in alone), 9.1 s after (two added images and the known-identity case). Hosted figures to compare with: 51.07 s on windows-arm64 and 54.22 s on the slowest earlier run (D1 run 43153144670); the 9.1/5.5 ratio puts the next hosted figure at roughly 85 s, so the timeout is now 180 s with the comment that it is provisional until a hosted run has the key-based cases. Plan 10 settles it.
- Runner calls the added cases cost: the skipped image 2 derived runs (export NTSC and PAL, facts discarded); the device image 0; the known-identity case 2 (the default row in the loop, the PAL profile) plus its probe; the seed itself adds one script run and two parses at start. The loop over the ledger's fixture rows for the two new images ran before (one is the old plain image).
- `compat.public_update`: untouched, 1.8 to 2.2 s locally (windows-arm64 47.73 s of 91 s in D1); `compat.ratchet` untouched. No test here reads the 1,473-line ledger: the stand-in is the nine fixture rows plus five seeded rows.

## Deviations

**1. [Design note, as the plan asked] "listed" means the loop ran the profile.** The plan's rule: a derived profile is skipped only for a profile the first loop ran, not when the ledger holds a row of the id. The probe's identity is empty for an image outside the compiled table (11-08 deviation 1), so for every synthetic image the loop over ledger rows does not run the seeded rows (it matches `fixture/NAME`, never a catalog SHA-1). A ledger-row test alone would therefore never flip a seeded row in the synthetic proof; the derived part runs those profiles and the update guard's existing key flips the seeded row. For a real cart the probe identity is the ledger id, the loop runs the default row, and the derived part skips only what the loop ran.

**2. [Choice] Skipped ids get derived facts, device ids none.** A skipped id has its derived runs (two runner calls for an export image) and the update guard discards the facts; a catalog id with a non-pad port among its ledger rows gets no derived profile at all. The first costs two runs a real skipped cart would not otherwise need but keeps the one rule "the guard is the only place a status is protected"; the second stops a pad run from ever satisfying a device title.

**3. [Rule 2 - missing coverage] Known-identity case added.** The plan's synthetic proof cannot reach the `ran` list (the identity is empty for owned images), so a mutation that never records a ran profile stayed green. A probe wrapper (the pattern of the flaky-image case) covers it; the case costs two runner calls and is a negative control above.

**4. [Rule 3] Planted names moved to the new images.** The plan said to add the two ids to the privacy checks; the skipped image's file name carries the home-path string and the device image's the address string, so the existing output loop covers them.

## Known Stubs

None.

## Threat Flags

None. The plan adds no endpoint, file access outside the build results directory or schema change.

## Self-Check: PASSED

- `scripts/library.cmake`, `tests/library/tests.cmake`, `.planning/seeds/SEED-013-pal-fm-library-run.md`, `.planning/seeds/SEED-009-scan-thresholds.md` exist and carry the changes above.
- Commits `6bdf8a9`, `e0a1374` and `9ec94e3` are ancestors of HEAD (`git merge-base --is-ancestor`).
