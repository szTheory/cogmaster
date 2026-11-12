---
phase: 12-whole-list-in-the-ledger
plan: "08"
subsystem: wording
tags:
  - readme
  - libretro-info
  - claims
  - experimental-flag
status: complete

requires:
  - 12-07
provides:
  - the README status paragraph, the libretro description and the conformance sentences scoped to the flag that stays, with no planning label and no count
  - policy.claims_scoped, a check that reads is_experimental from the template and ties the wording to it in both states
  - policy.claims_scoped.self_test, eleven trees built from the real files (nine faults and controls, two passing positives besides the real tree)
affects:
  - 12-09
  - 12-10

actuals:
  tokens: 3800
  tasks: 2
  commits: 2

plan_head_before: 4280970aa4d1554ec033dbf4e877f81bc964b623
plan_head_after: 3da1a0c0da0eac118c99bdd88310c2f06fd8e4cc
commits: 2

tech-stack:
  added: []
  patterns:
    - "A user-text guard reads the flag it is tied to from the file that carries it and judges the wording in both states, so removing the flag forces every sentence to go in the same change"
    - "A page that prints catalog titles verbatim is checked by its non-table lines only, copied to a temporary file named like the page so a message names the page"
    - "Fixture trees that carry the character at-sign are written through file(CONFIGURE ... NEWLINE_STYLE UNIX) after each at-sign is replaced by a defined variable that expands to it"

key-files:
  created:
    - tests/policy/claims_cases.cmake
  modified:
    - README.md
    - libretro/cogmaster_libretro.info.in
    - docs/conformance.md
    - tests/cmake/repository.sh
    - tests/policy/tests.cmake
    - tests/policy/repository-checks.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt

key-decisions:
  - "The flag stays (owner decision of 2026-11-12), so every user-facing site says experimental, promises nothing about a game, and states no count that a test cannot keep fresh"
  - "The claims check is a literal tripwire, stated as such: a reworded over-claim passes; it exists to force the wording to move with the flag"
  - "The page is read by its non-table lines because a table row prints a catalog title verbatim and three pinned titles hold the planning label's text as part of the game's own version"

requirements-completed: []

coverage:
  - id: C1
    description: "The README status paragraph says experimental, promises nothing about a single game, says most catalogued titles are listed unknown, calls the checks narrow, points to the page for counts, names what is not supported and says Mark III software is not tracked there; line 4 still says the core covers the Mark III; the paragraph holds no number, no planning label and neither dropped sentence"
    requirement: LEDG-05
    verification:
      - kind: integration
        ref: "grep acceptance commands of the plan (Status: experimental present; covers the Mark III present; Mark III cartridges absent; Mark III software is not tracked present; the digit rule prints 0; the two dropped sentences absent) all as expected; ctest -R '^(readme|libretro\\.info_claims|policy\\.|docs\\.|compat\\.exit)' passes"
        status: pass
    human_judgment: false
  - id: C2
    description: "The libretro description reads as the plan states, names no planning label, and the is_experimental line is unchanged; compat.exit still passes against the edited template (flag present, verdict unmet) and libretro.info_claims and its self tests are unchanged"
    requirement: LEDG-05
    verification:
      - kind: integration
        ref: "grep -c '^is_experimental = \"true\"$' prints 1; ctest -R '^(compat\\.exit|libretro\\.info_claims)' passes; git diff shows no line of the claims exemption edited"
        status: pass
    human_judgment: false
  - id: C3
    description: "docs/conformance.md says the template keeps is_experimental until the judged-cart floors are met and that compat.exit enforces it both ways, drops the planning label, and describes the library judge's adoption by (id, profile); the file is 34,306 bytes, 190 above the previous size"
    requirement: LEDG-05
    verification:
      - kind: other
        ref: "test $(wc -c < docs/conformance.md) -le 34600 exits 0 and docs.budgets passes; the diff adds no planning id (grep for LEDG-0, DEC., Phase N, Plan N, SEED-N exits 1 on the added lines)"
        status: pass
    human_judgment: false
  - id: C4
    description: "policy.claims_scoped passes on the real tree with the flag present and, in its self test, refuses a README that plays 900 games, the flag present with a README that lacks the word, the flag absent with a README, a docs file or the template saying it, a description holding the planning label, and a stale description without the word; it accepts a flag-absent tree and a page whose table row holds a catalog title with the planning label's text, and refuses the same text on a prose line, naming the page"
    requirement: LEDG-05
    verification:
      - kind: unit
        ref: "ctest -R '^policy\\.claims_scoped' passes; the self test prints policy.claims_scoped: 11 controls"
        status: pass
    human_judgment: false
  - id: C5
    description: "The check can fail: four mutations of the check each turn the registered tests red and restore green (over-claim pattern dropped, flag-absent branch dropped, digit rule made loose so the README sentence about glasses titles is refused, page read whole so the Graphic Board row is refused)"
    requirement: LEDG-05
    verification:
      - kind: unit
        ref: "python edit of tests/cmake/repository.sh, git diff --stat, ctest -R '^policy\\.claims_scoped' exit 8, restore, exit 0 (four times); the loose rule and the whole-page read each also fail on the real tree"
        status: pass
    human_judgment: false
  - id: C6
    description: "The counted check code is 885 (limit 940, ceiling 905), six lines above the previous figure; the inventories gain exactly two names each in ci.txt and wasm32.txt and none in hygiene.txt; the case is only added"
    requirement: LEDG-05
    verification:
      - kind: other
        ref: "cmake -DROOT=$PWD -P tests/cmake/budgets.cmake exits 0 printing 885 check lines; git diff --numstat origin/main on the three inventories shows 14/0, 14/0 and 10/0; git diff -U0 origin/main -- tests/cmake/repository.sh removes no line"
        status: pass
    human_judgment: false
  - id: C7
    description: "The owner's own-library run and the removal of is_experimental in the pull request that merges its rows stay the batched hand-off of SEED-013, never a gate; claims_scoped now forces the wording out in that same pull request"
    requirement: LEDG-05
    verification:
      - kind: manual_procedural
        ref: .planning/seeds/SEED-013-pal-fm-library-run.md
        status: not_run
    human_judgment: true
---

# Phase 12 Plan 08: Scoped wording and the claims tie Summary

The README, the libretro description and the conformance text now say experimental, promise nothing about any single game and carry no planning label or count, and a test keeps them in step with the flag: with the flag present the README, the description and the page's prose must say the word; with it absent none of the user text may. The flag line is untouched.

## Performance

- **Tasks:** 2 of 2 (task 1 is the tracer: the wording, end to end through the readme, policy, docs and compat.exit tests)
- **Commits:** `e1d5a14` (task 1), `3da1a0c` (task 2)
- **Counted check code:** 885 (previous 879, plan cap 8, ceiling 905). Printed: `885 check lines / 9403 src lines, strict one tenth`. The case is six physical lines in `tests/cmake/repository.sh`; the cases script is classified `behavior` and not counted.
- **Docs bytes:** conformance 34,306 (was 34,116; at most 34,600 and at most 300 above), embedding 36,521 (unchanged; not edited), README 4,908 (was 4,493 on origin/main; cap 12,288). `docs/saves.md` and `docs/embedding.md` keep their sentences.
- **Inventories:** `git diff --numstat origin/main` shows 14 added and 0 deleted in `ci.txt` and `wasm32.txt`, 10 added and 0 deleted in `hygiene.txt`.

## Accomplishments

1. **Wording (task 1).** README Status paragraph as the owner's wording and the plan: experimental, no promise about a game, most catalogued titles listed unknown, narrow checks (boots, a frame matches, a value appears), counts and rows on the page, not supported: cartridges for the SG-1000, save states and the unsupported boards, and "the page covers Master System and Game Gear cartridges only, so Mark III software is not tracked there". A second short paragraph keeps the two old checkable facts (the core option `FM sound unit`, the built-in test card). Line 4 (the core covers the Mark III) is unchanged. The description in the template is the plan's sentence. Conformance: the flag sentence names `is_experimental`, the floors and `compat.exit` both ways; "through" the planning label is gone; the library judge paragraph describes adoption by (id, profile) (a seeded default row flips, a missing PAL or Japanese FM profile is added, device, skipped and unsupported rows are never overwritten, a fact for a skipped identity is discarded).
2. **The claims check (task 2).** `claims_scoped` in `tests/cmake/repository.sh` reads the flag from the template (a line that is exactly `is_experimental = "true"`), reads the page through its non-table lines, and applies the planning-label, over-claim and digit rules to the README and the template, plus the state rules above. Every message names the file or the rule.
3. **The self test (task 2).** `tests/policy/claims_cases.cmake` builds each tree from the real README, page, `docs/saves.md` and template with one string edit, runs the check as a child through `repository.cmake`, and asserts exit status and a tolerant message regex. `policy.claims_scoped.self_test` prints `policy.claims_scoped: 11 controls`.

## Confirmed before writing "every platform"

- `grep -q 'savestate = "false"' libretro/cogmaster_libretro.info.in` exit 0 (so the README lists save states as not supported).
- `grep -q 'TUNE-10' .planning/REQUIREMENTS.md` exit 0 and the traceability row reads `| TUNE-10 | Phase 5 | Complete |`.
- Both held, so the README keeps the plan's phrasing.

## Evidence (command, exit status; local, repository root)

- `cmake --preset ci > /dev/null && ctest --test-dir build/ci -R '^(readme|libretro\.info_claims|policy\.|docs\.|compat\.exit)' --output-on-failure` exit 0 (62 tests) and `cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0, after task 1.
- `cmake --preset ci > /dev/null && cmake --build --preset ci && ctest --test-dir build/ci -R '^(policy\.claims_scoped|libretro\.info_claims|compat\.exit|policy\.notices)' --output-on-failure` exit 0 (8 tests) and the budget command exit 0, after task 2.
- `ctest --test-dir build/ci -R '^policy\.claims_scoped\.self_test$' -V` exit 0, log matches `policy.claims_scoped: 11 controls`.
- `cmake --workflow --preset ci` exit 0 (666 of 666 tests, two more than before) and `cmake --workflow --preset hygiene` exit 0 (84 of 84), both on the committed state of task 2 (the inventory change makes ci 666).
- Task 1 acceptance greps: no planning label in the README or the template (`grep -c` prints 0 for both), none in the page's non-table lines (exit 0); `grep -c '^is_experimental = "true"$'` prints 1; `covers the Mark III` found; `Mark III cartridges` not found; `Mark III software is not tracked` found; `Status: experimental` found; the digit rule prints 0 on the README; the two dropped sentences are absent; no planning id in the added text lines of `git diff -U0 origin/main -- docs README.md libretro`.
- `git diff -U0 origin/main -- tests/cmake/repository.sh` removes no line (exit 0 on the negation).
- `gcc:13` container check: no C file was touched; not applicable.
- No hosted run in this plan (D2 is plan 10); no run URL.

## Negative controls (each: python edit of `tests/cmake/repository.sh`, `git diff --stat`, `ctest -R '^policy\.claims_scoped'`, restore, rerun)

| Control | Mutation | Result |
|---|---|---|
| Over-claim pattern dropped | the alternation replaced by a word that never matches | ctest exit 8, the self test red (the `over_claim` control); restored exit 0 |
| Flag-absent branch dropped | the condition made `true \|\| ...` so the present branch always runs | ctest exit 8, the self test red (`flag_absent_*`); restored exit 0 |
| Loose digit rule | `[0-9].{0,12}(games\|titles\|cartridges\|carts)` | ctest exit 8, both tests red; the real tree fails with `README.md over-claims` on the sentence about glasses titles; restored exit 0 |
| Page read whole | `cat` of the page in place of the non-table filter | ctest exit 8, both tests red; the real tree fails with `compatibility.md names a planning label` on the Graphic Board row; restored exit 0 |

After the last restore `git diff --stat tests/cmake/repository.sh` shows 6 insertions and the real tree exits 0.

## Deviations

**1. [Rule 1 - acceptance conflict] "SG-1000 cartridges" would match the digit rule.** The plan's README text says "Not supported: SG-1000 cartridges", but its own rule (a whole number, a space, at most two words, then cartridges) matches "1000 cartridges" in it, so the acceptance check (digit rule prints 0) and the guard on the real tree would fail. The README reads "cartridges for the SG-1000" instead; the meaning is the plan's.

**2. [Design note] The page is read through a temporary directory named like the page.** `claims_scoped` copies the non-table lines to `<tmpdir>/compatibility.md`, so a message naming the planning label on a prose line names the page (the plan requires "naming the page"), and the directory is removed on exit.

**3. [Design note] The registration is in `tests/policy/tests.cmake`, not a lane-specific file.** The check and its self test run on every lane that runs the policy tests (ci and wasm32 inventories). The built `.info` is the template with one substitution (`display_version`), so the template is the file scanned; there is no separate built-file read.

**4. [Scope note] 11 runs, not 7.** The self test covers the real files and the two passing positives (a flag-absent tree, a catalog-title table row) beside the eight refusals (the five of the plan, the same title on a prose line, and two flag-absent trees where a docs file or the template still says the word).

**5. [Design note] The check counts six lines, not eight.** The over-claim and planning-label tests share one loop and the two flag states share one `if`, so the plan's allowance of eight was not needed.

## Known Stubs

None.

## Threat Flags

None. The check reads repository files and writes only a temporary directory it removes; the self test writes only its scratch directory.

## Self-Check: PASSED

- `README.md`, `libretro/cogmaster_libretro.info.in`, `docs/conformance.md`, `tests/cmake/repository.sh`, `tests/policy/claims_cases.cmake`, `tests/policy/tests.cmake`, `tests/policy/repository-checks.txt` and the two inventories exist and carry the changes above.
- Commits `e1d5a14` and `3da1a0c` are ancestors of HEAD (`git merge-base --is-ancestor`).
