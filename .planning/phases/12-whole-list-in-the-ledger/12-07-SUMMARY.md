---
phase: 12-whole-list-in-the-ledger
plan: "07"
subsystem: exit-command
tags:
  - compat
  - exit
  - experimental-flag
  - library
status: complete

requires:
  - 12-06
provides:
  - tests/compat/exit.cmake, the exit command over the ledger's judged rows, with the flag and ledger tie in both directions
  - six reviewed floors with reasons in quality.json (three over the own fixtures, three over judged cartridge rows)
  - compat.exit on the committed ledger and the real template, and compat.exit.self_test with the met branch, the tie both ways, edges and integrity controls
  - the exit block printed at the end of scripts/library.sh, a failure printed and never a gate
affects:
  - 12-08
  - 12-09
  - 12-10

actuals:
  tokens: 10200
  tasks: 3
  commits: 3

plan_head_before: 44a5f3520ccf10489b10bbe9d110b6172e3899a5
plan_head_after: 1d58f83ca3c63097cc6227cee8a041334f945baf
commits: 3

tech-stack:
  added: []
  patterns:
    - "A command is one CMake function that sets a status, a message and printed lines and never exits or prints; the script is a thin wrapper and the self test calls the function in process, so one control costs two awk spawns instead of a cmake child"
    - "The in-memory cases text of a control is derived from the rows that write its ledger file, and one guard proves it equal to the real parser's output before any control relies on it"
    - "A stand-in with no pin counts gets them derived from its own catalog by the same awk that reads the catalog once, so only the arithmetic is skipped and the identity sets are still compared both ways"

key-files:
  created:
    - tests/compat/exit.cmake
  modified:
    - tests/compat/tests.cmake
    - tests/library/tests.cmake
    - scripts/library.sh
    - quality.json
    - tests/policy/repository-checks.txt
    - tests/inventory/ci.txt
    - tests/inventory/wasm32.txt
    - tests/inventory/hygiene.txt

key-decisions:
  - "An unmet verdict is not a failure; a broken claim is, and so is a flag that disagrees with the verdict, so the pull request that merges the owner's rows is red until the flag and its wording go in the same change"
  - "The flag is detected as a line that sets is_experimental to true with any spacing or quoting, so a respelled line cannot make the tie vacuous, and a comment that merely mentions it does not count"
  - "Judged cartridge rows are pass or fail rows keyed by an image identity of the catalog; a fixture id and a derived compound id are never counted, even when a catalog does not mark the compound entry skipped"
  - "The command prints counts only; a fault is printed once, by the final error, and library.sh takes the counts lines and the first error line"

requirements-completed: []

coverage:
  - id: C1
    description: "The exit command on the committed ledger and the real libretro template exits 0 and prints counts only: 1,472 rows, fixture_pass=9 pal_judged=1 fm_judged=2, cart_pass=0/20 cart_pal_judged=0/1 cart_fm_judged=0/1, judged_cart_floors=unmet flag=present, unowned_unknown=1434"
    requirement: LEDG-05
    verification:
      - kind: integration
        ref: "cmake -DLEDGER=tests/compat/ledger.csv -DINFO=libretro/cogmaster_libretro.info.in -P tests/compat/exit.cmake exit 0; ctest -R '^compat\\.exit$' passes; negative control: the verdict forced to met turns it red with judged-cart floors met: remove is_experimental, restored green"
        status: pass
    human_judgment: false
  - id: C2
    description: "The five claim-integrity legs each end the run with their leg and the count named: a parser refusal, a reason that is not the catalog's or not open, a fixture fail or an infrastructure_error row, a fixture floor under today's value, a ledger identity set that differs from the catalog's or the pin counts, a row count over half the parser cap"
    requirement: LEDG-05
    verification:
      - kind: unit
        ref: "compat.exit.self_test: empty evidence through the real parser, fixture fail, infrastructure_error, PAL fixture removed, fixture pass removed, status outside the enum, floor missing, reason not the catalog's, retired reason, identity missing, pin counts one low; negative control: leg 3 removed turns it red, restored green"
        status: pass
    human_judgment: false
  - id: C3
    description: "The verdict is not constant: twenty judged cartridge passes with one PAL and one Japan FM row and the flag gone reach met and exit 0, nineteen passes, no PAL row, no FM row, twenty fixture passes and twenty passes keyed by compound identities stay unmet, and extra cartridge fail rows never block"
    requirement: LEDG-05
    verification:
      - kind: unit
        ref: "compat.exit.self_test: met, unmet_flag_present, no_pal, no_fm, twenty_fixture_passes, twenty_compound_passes, cart_fail_rows; negative control: the cart_pass floor read as 0 turns it red, restored green"
        status: pass
    human_judgment: false
  - id: C4
    description: "The flag is tied to the verdict both ways: floors met with the flag present (also as another spelling) exits 1 with judged-cart floors met: remove is_experimental, and floors unmet with the flag absent exits 1 with is_experimental removed before the judged-cart floors are met; the exit status is proven end to end on a real file by the single child process"
    requirement: LEDG-05
    verification:
      - kind: unit
        ref: "compat.exit.self_test: tie_met_flag_present, tie_met_flag_spaced, tie_unmet_flag_absent, no_template and the one cmake child (exit 1, tie message); negative control: the tie ignoring the flag-present case turns it red, restored green"
        status: pass
    human_judgment: false
  - id: C5
    description: "The in-memory cases text every control relies on equals the parser's output for the same ledger in the four fields the command reads"
    requirement: LEDG-05
    verification:
      - kind: unit
        ref: "compat.exit.self_test format guard; negative control: one field of the helper text altered turns it red at the guard, restored green"
        status: pass
    human_judgment: false
  - id: C6
    description: "Six floors with six distinct reasons are in quality.json and no existing key changed"
    requirement: LEDG-05
    verification:
      - kind: unit
        ref: "policy.quality passes; jq -e on the six floor keys, the six reasons keys and the distinct reasons exit 0; git diff -U0 origin/main -- quality.json removes no tests. line"
        status: pass
    human_judgment: false
  - id: C7
    description: "sh scripts/library.sh run ends by printing the exit block over the merged ledger; the merged synthetic ledger is judged unmet; a failing exit command is printed with its status and first message line and leaves the script status at 0"
    requirement: LEDG-05
    verification:
      - kind: integration
        ref: "library.synthetic: exit: cart_pass=N/20 and judged_cart_floors=unmet in the applied run, the command over the merged stand-in ledger exits 0 unmet, an infrastructure_error row prints exit: failed (1) and the leg while library.sh exits 0; negative control: library.sh exiting with the command's status turns it red (synthetic library operation failed: 1), restored green"
        status: pass
    human_judgment: false
  - id: C8
    description: "The owner's one own-library run (sh scripts/library.sh run with their library and BIOS folders) and the removal of is_experimental in the pull request that merges its rows stay the batched hand-off of SEED-013, never a gate; compat.exit forces the removal in that pull request when the printed block says judged_cart_floors=met"
    requirement: LEDG-05
    verification:
      - kind: manual_procedural
        ref: .planning/seeds/SEED-013-pal-fm-library-run.md
        status: not_run
    human_judgment: true
---

# Phase 12 Plan 07: The exit command and the flag tie Summary

The exit command exists and is proven in CI: it reads the committed ledger through the shared parser, runs the claim-integrity legs, counts judged cartridge rows against six reviewed floors and ties the verdict to `is_experimental` in both directions, with the met branch exercised on a stand-in ledger. Today the verdict over the committed ledger is unmet (0 of 20 judged cartridge passes, no PAL or FM cartridge row), so the flag stays and the command exits 0.

## Performance

- **Tasks:** 3 of 3 (task 1 is the tracer: the command on the committed ledger and the real template, end to end)
- **Commits:** `4bae8c9` (task 1), `e2d4113` (task 2), `1d58f83` (task 3)
- **Counted check code:** 879, unchanged (plan cap 897; `exit.cmake` is behavior, `library.sh` is operation, `quality.json` and the tests are not counted). Printed: `879 check lines / 9403 src lines, strict one tenth`.
- **Docs bytes:** nothing under `docs/` or the README was touched (`git diff --stat 44a5f35 HEAD -- docs README.md` is empty). Current sizes: maintaining 19,862, conformance 34,116, embedding 36,521, testing 32,057, runner 34,048, boards 9,221; every file under the cap 38,912.
- **Ledger and page:** `git diff --quiet ea47145 -- tests/compat/ledger.csv docs/compatibility.md` exit 0 (plan 5's final commit, `ea47145`, is the base).
- **Inventories:** `git diff --numstat origin/main -- tests/inventory/ci.txt tests/inventory/wasm32.txt tests/inventory/hygiene.txt` shows 12 added and 0 deleted in `ci.txt` and `wasm32.txt` and 10 added and 0 deleted in `hygiene.txt`.

## Exit block on the committed ledger

The command: `cmake -DLEDGER=tests/compat/ledger.csv -DINFO=libretro/cogmaster_libretro.info.in -P tests/compat/exit.cmake` from the repository root. Exit status 0. Printed lines (counts only; this is what the owner sees before their run):

```
exit: rows=1472 pass=9 fail=0 unknown=1434 unsupported=5 skipped=24 infrastructure_error=0
exit: fixture_pass=9 pal_judged=1 fm_judged=2
exit: cart_pass=0/20 cart_pal_judged=0/1 cart_fm_judged=0/1 judged_cart_floors=unmet flag=present
exit: unowned_unknown=1434 cart_pal_rows=0 cart_fm_rows=50
exit: pass
```

`grep -Eq` on `judged_cart_floors=unmet flag=present`, `fixture_pass=9`, `unowned_unknown=1434` and `exit: pass` each exit 0.

## Accomplishments

1. **The command (task 1).** `tests/compat/exit.cmake` is one function, `compat_exit_check`, that takes the ledger, catalog, pin counts (a path or `none`), reasons, the floors as JSON, a template path and an optional in-memory cases text, and returns a status, a message and the printed lines without exiting or printing. The wrapper prints the lines and turns a fault into a failing exit. It runs the parser's `normalize` (leg 1) and `cases` once each, reads the catalog once by one awk into CMake sets (image identities, the distinct reason tokens, derived counts), tallies four fields of every row in one `foreach`, and calls the shared reason and count legs of `whole_list_common.cmake` for legs 2 and 5. The floors are read with `string(JSON)`; a missing key is a fault naming it.
2. **The floors (task 1).** `quality.json` gains `compat.judged_pass` 9, `compat.pal_judged` 1, `compat.fm_judged` 2, `compat.cart_pass` 20, `compat.cart_pal_judged` 1 and `compat.cart_fm_judged` 1, each with its own reasons entry (the three fixture floors are today's measured values; the three cartridge floors show a real library run reached the ledger and are not a pass rate). No existing key changed.
3. **The tie (task 1).** With a template the verdict is compared with the flag line: met with the flag present exits 1 `judged-cart floors met: remove is_experimental`; unmet with the flag absent exits 1 `is_experimental removed before the judged-cart floors are met`. `compat.exit` runs the committed ledger against the real template; it accepts exactly the two consistent states (unmet with the flag present, met with it absent), so the pull request that merges the owner's rows is red until the flag and its wording go in the same change. `repository.sh` and its exemption were not edited.
4. **The self test (task 2).** `compat.exit.self_test` runs 22 controls in process over a stand-in catalog of 20 generated image identities and one compound identity, the nine fixture rows copied from the live ledger and generated cartridge rows. It starts one cmake child (the command on a real met ledger with a flag-present stub, expecting exit 1 and the tie message) and prints `compat.exit.self_test: 22 controls, 1 child`. The real parser reads a ledger three times in total: the format guard, the empty-evidence control (exit 2 is leg 1) and inside the child.
5. **The library block (task 3).** `scripts/library.sh run` appends the exit lines of the command over the merged ledger to `summary.txt` before it prints it; a failing command adds `exit: failed (<status>)` and its first message line; the script's own status ignores it. A stand-in catalog gets `-DCOUNTS=none`. `library.synthetic` asserts the unmet verdict in the applied run and over the merged stand-in ledger, and that an `infrastructure_error` row prints `exit: failed (1)` with `leg 3` while the script exits 0.

## Evidence (command, exit status; local, repository root)

- `cmake --preset ci > /dev/null && cmake --build --preset ci && ctest --test-dir build/ci -R '^(compat\.exit|policy\.quality|compat\.whole_list)' --output-on-failure && cmake -DROOT="$PWD" -P tests/cmake/budgets.cmake` exit 0 (16 tests passed) after task 1.
- `ctest --test-dir build/ci -R '^compat\.exit' --output-on-failure` and `ctest --test-dir build/hygiene -R '^compat\.exit' --output-on-failure` exit 0 after task 2; `ctest --test-dir build/ci -R '^compat\.exit\.self_test$' -V` prints `compat.exit.self_test: 22 controls, 1 child`.
- `ctest --test-dir build/ci -R '^(library|compat\.exit)' --output-on-failure` exit 0 after task 3.
- `cmake --workflow --preset ci` exit 0 (664 of 664 tests passed) and `cmake --workflow --preset hygiene` exit 0 (84 of 84), both on the committed state of task 3.
- `jq -e '[.floors | keys[] | select(startswith("compat."))] | length == 6' quality.json`, `jq -e '[.reasons[].key | select(startswith("compat."))] | length == 6' quality.json` and `jq -e '[.reasons[].reason] | length == (unique | length)' quality.json` exit 0; `git diff -U0 origin/main -- quality.json` removes no `tests.` line (exit 0).
- `ctest --test-dir build/ci -N -R '^compat\.exit$' | grep -q 'Total Tests: 1'` exit 0; `grep -c 'exit.cmake' scripts/library.sh` prints 1.
- Awk portability: the catalog awk of the command (the only new awk; the reason and count legs are the plan 5 files, unchanged) prints identical sorted output on macOS awk, mawk 1.3.4 (`docker run ubuntu:26.04`) and gawk (installed in the throwaway container only) over the 1,677-line catalog (md5 `4ffa27b52d5a38157b11ba4ebf836096` for all three).
- `gcc:13` container check: no C file was touched; not applicable.
- No hosted run in this plan (D2 is plan 10); no run URL.

## Negative controls (each: python edit, `git diff --stat`, `ctest -R '^compat\.exit'`, restore, rerun)

| Control | Mutation | Result |
|---|---|---|
| Verdict constant (task 1) | `set(verdict unmet)` made `set(verdict met)` | compat.exit exit non-zero: `judged-cart floors met: remove is_experimental`; restored: 100 percent passed |
| Tie ignores the flag-present case | the met-and-present condition made `if(FALSE)` | compat.exit.self_test non-zero: `the tie_met_flag_present control ended with status 0, not 1`; restored green |
| cart_pass floor read as 0 | the floor assigned 0 after reading | compat.exit.self_test non-zero (the nineteen-pass control and the edges reach met); restored green |
| Leg 3 removed | the fixture-fail and infrastructure-error conditions made `if(FALSE)` | compat.exit.self_test non-zero: `the fixture_fail control did not match leg 3`; restored green |
| In-memory cases text altered | the fm field of the helper text takes the tv value | compat.exit.self_test non-zero at the format guard: `the in-memory cases text differs from the parser's`; restored green |
| library.sh as a gate (task 3) | `[ "$status" -eq 0 ] \|\| exit "$status"` after the summary | library.synthetic non-zero: `synthetic library operation failed: 1`; restored green |

For the format guard the guard was moved ahead of the first control so that this mutation turns red at the guard itself and not at a count.

## Cost and timing (local; no claim about hosted time)

- `compat.exit` 0.42 to 0.55 s slowest of 6 runs; `compat.exit.self_test` 0.20 to 0.32 s. Each control costs two awk spawns for the shared legs (the failing integrity controls cost none, because they end before those legs); the catalog awk runs once per distinct stand-in catalog.
- `library.synthetic` 10.6 to 11.1 s after this plan (10.2 to 10.7 s before; 15 to 17.5 s in two runs under load while the full workflow was also running). Each of its nine `library.sh run` calls now makes one more `cmake -P` of the exit command (about six spawns on a stand-in ledger of 14 rows). On windows-arm64 (hosted 51 s before this phase's additions, timeout 180 s) a spawn costs 15 to 25 times a native one; plan 10 reads the hosted figure and settles the timeout. The live ledger is parsed by `compat.exit` once per lane and by nothing added here.

## Deviations

**1. [Design note] The self test lives in `exit.cmake` itself.** The plan describes one include of the function part; the file has a `self_test` CASE next to the function, so it needs no include, and the single child runs `cmake -P` on the same file. The count line still prints `1 child`.

**2. [Design note] The format guard compares the four fields the command reads.** The plan says byte for byte. The in-memory text carries `id`, `tv`, `status` and `fm` of every row only (the parser prints about 18 fields), so the guard compares the same four fields of the parser's output with the helper's text, in the parser's order (the rows are sorted as the parser sorts, and the guard proves it). A drift in any of those four, in naming or values, turns it red; the mutation above shows it.

**3. [Design note] `-DCOUNTS=none` derives the counts from the catalog.** The same awk that reads the catalog once also counts its lines by kind and writes a pin-counts file for the shared leg, so the arithmetic is tautological and only the identity sets (ledger against catalog, both ways), the compound and image counts and the row cap are compared. A real run with the tracked catalog never takes this path.

**4. [Design note] The reason token must be open is checked on the catalog's tokens.** Leg 2 compares each ledger reason with its catalog token (the shared leg) and then requires every distinct reason token of the catalog to be open in the vocabulary; a ledger reason is always one of those, so this is the plan's rule for rows, stated once per token.

**5. [Design note] A fault is printed once.** The wrapper prints the counts lines with `message(STATUS)` and the fault only through the final error; `library.sh` takes the counts lines from the status output and the first error line when the command fails, so no message is duplicated.

**6. [Scope note] 22 controls, not 16.** The cheap integrity controls end before any awk spawn (they cost almost nothing), which allowed more than the 14 the plan requires; two extra ones guard the flag detection (another spelling of the flag line counts as present, and a template with no flag line but a comment naming it counts as absent) and one the omitted template (`flag=unchecked`).

**7. [Rule 3] The ctest assertion accepts both consistent states.** The plan's regex was `judged_cart_floors=unmet flag=present`; the registered test accepts `unmet flag=present` or `met flag=absent` so that the owner's removal pull request, which is consistent by construction, does not fail the test on the wording of the assertion. The acceptance command recorded above still matches the first form.

## Known Stubs

None.

## Threat Flags

None. The command reads repository files and writes only to a scratch directory; `library.sh` writes the command's output (counts only) to `exit.txt` in its ignored results directory and prints only lines the command marks as `exit:`.

## Self-Check: PASSED

- `tests/compat/exit.cmake`, `quality.json`, `scripts/library.sh`, `tests/library/tests.cmake` and `tests/compat/tests.cmake` exist and carry the changes above.
- Commits `4bae8c9`, `e2d4113` and `1d58f83` are ancestors of HEAD (`git merge-base --is-ancestor`).
