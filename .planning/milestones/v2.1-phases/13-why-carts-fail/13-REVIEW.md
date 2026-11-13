---
phase: 13-why-carts-fail
reviewed: 2026-11-12T00:00:00Z
depth: standard
files_reviewed: 23
files_reviewed_list:
  - ASSET_POLICY.md
  - CMakeLists.txt
  - docs/conformance.md
  - docs/runner.md
  - docs/testing.md
  - runner/args.c
  - runner/args.h
  - runner/diag.c
  - runner/diag.h
  - runner/diag_class.h
  - runner/expect.c
  - runner/expect.h
  - runner/main.c
  - runner/probe.c
  - scripts/library.cmake
  - scripts/library.sh
  - tests/inventory/ci.txt
  - tests/inventory/wasm32.txt
  - tests/library/diagnose.cmake
  - tests/library/tests.cmake
  - tests/runner/diagnose.cmake
  - tests/runner/test_runner.c
  - tests/runner/tests.cmake
findings:
  critical: 0
  warning: 3
  info: 5
  total: 8
  necessity: 1
status: issues_found
---

# Phase 13: Code Review Report

**Reviewed:** 2026-11-12
**Depth:** standard
**Files Reviewed:** 23
**Status:** issues_found

## Summary

Reviewed the `--diagnose` runner mode (`runner/diag*.{c,h}`, the `expect_observe` split, args and main wiring), the owner-facing reducer in `scripts/library.sh diagnose` with its per-cart orchestration in `scripts/library.cmake`, the docs and the tests.

The C side is sound. I traced the frame-600 verdict (the same `cogmaster__expect_boots` the `--expect` path uses), the 0-based fault frame against the 1-based call numbers in `cogmaster__diag_order` and `cogmaster__diag_bucket`, the hash-set bookkeeping, buffer sizes, and every allocation and free path in `run()`. I found no defects there. The awk reducer is a closed-grammar validator. I checked field numbering (25 fields), the sort-key layout against the `cut -f` offsets, the ledger parse arithmetic and the class precedence. Every fail row lands in exactly one group, so `groups_total` equals `fail_total` for any record that passes validation. No per-cart path, byte, digest or file name can reach stdout, and the catalog title is gated by a charset and a length check.

No finding reaches BLOCKER. The three warnings are:
- a documentation claim that the output holds "no exact frame" when three exact frame numbers are printed;
- a `ram=live|still` signal that can measure the open bus instead of work RAM;
- a cleanup trap that does not cover the common interrupt case, which leaves digest-bearing scratch files behind.

The test and tooling observations are listed as seeds per project rule 7. The 6 hex-leak and canary tests give the scan real teeth, and I found no vacuous test.

## Warnings

### WR-01: The diagnose line is documented as free of "exact frame" but prints three exact frame numbers

**File:** `runner/diag.h:1-3`, `docs/runner.md` (Diagnose section, first paragraph)
**Issue:** The header comment says the line "leaks no digest, opcode, exact frame, size or name". `docs/runner.md` says the line holds "never a digest, opcode, exact frame, image size, region guess, name or path". The line prints `first_display=`, `first_nonblank=` and `first_change=` as exact run-call numbers (`first_display=2` in the doc's own example). `pc=` is also an exact address, and `scripts/library.sh` prints all of these per row next to the title. `ASSET_POLICY.md` correctly lists "frame numbers, fault PCs" as printed, so the policy file and the code agree, and the header and user doc contradict both. The only frame that is bucketed is the fault frame. A reader who trusts the header will conclude the numbers are safe to paste anywhere, and a later change may add an exact fault frame on the strength of that claim.
**Fix:** State what the line holds, as `ASSET_POLICY.md` does. For `diag.h`: "Every printed value is a number, an enum word or a word from a static table, so the line leaks no digest, byte, image size, name or path. It does print the first-event call numbers and the fault pc; the fault's own frame is bucketed." Apply the same wording in `docs/runner.md`, and drop "exact frame" and "region guess" from the "never" list unless region guess is truly absent (it is: the record carries the region but the line does not).

### WR-02: `ram=live|still` can report the open bus instead of work RAM

**File:** `runner/diag.c:48-54`, `runner/diag.c:76-82` (with `src/bus.c:145-151` and `src/bus.c:203-212`)
**Issue:** `ram_read` samples `$C000..$DFFF` with `cogmaster_peek(m, COGMASTER_SPACE_BUS, ...)`. When the program has turned work RAM off in `$3E` (`BUS_RAM_OFF`, bit 4), `mem_slow_driven` returns 0 for every address. `cogmaster_peek` then returns `m->bus.data | bus_pullup`, which is the last value on the CPU bus, not RAM. A jammed or idle program that disables RAM and spins therefore compares 8 KiB of "last bus byte". The two samples at calls 1740 and 1800 can differ (`live`) or match (`still`) with no relation to whether any program state changed. `ram` is the only input to the `static_no_fault` split (`frozen` versus `alive_still`, library.sh line 144), so a cart in this state is filed under a cause on noise. `$3E` writes that disable RAM are exactly what a bad init sequence or a mis-banked cart does.
**Fix:** Tell RAM from open bus in the sample. `cogmaster_peek` returns the same value for every undriven address, so the sample is open bus when all 8192 bytes are equal and the work RAM state cannot be told apart. The cheapest guard is to treat a sample whose 8192 bytes are all one value as unknown and report `still` only when both samples are identical and not uniform:
```c
/* uniform: every byte equals the first; peek cannot tell RAM from a driven-off bus. */
static int uniform(const uint8_t *p, size_t n)
{
    for (size_t i = 1u; i < n; i++)
        if (p[i] != p[0])
            return 0;
    return 1;
}
```
Then `d->live = memcmp(...) != 0 && !(uniform(now) && uniform(d->ram))`. A fresh-powered RAM is not uniform on this core only if the core randomises it; check `src/` for how RAM is initialised before choosing the guard, and add a library.diagnose case with a seed that writes `$3E` to turn RAM off. A public non-side-effect "work RAM enabled" bit would be cleaner than guessing, but that is an API change for a later phase.

### WR-03: The scratch directory is removed only on a normal exit, and it holds per-row data and cart identity digests

**File:** `scripts/library.sh:41-44` (and `docs/testing.md`, the diagnose paragraph)
**Issue:** `trap 'rm -rf "$tmp"' EXIT` runs on a normal or `set -e` exit only. In POSIX `sh` as implemented by dash (the default `/bin/sh` on Debian and Ubuntu, also used by CI runners) an `EXIT` trap does not run when the shell dies from SIGINT, SIGTERM or SIGHUP. A diagnose run is one 1800-frame emulation per candidate plus a cmake process per cart, so a library of hundreds of carts takes long enough that the owner will press Ctrl-C. `docs/testing.md` documents only `SIGKILL` as the exception. The directory it leaves behind, `build/library/diag.XXXXXX`, holds `records` (the 40-hex identity SHA-1 of every owned dump, plus the cart's board and run result) and `cases` (a copy of the ledger). Policy line "never prints ... digests" is about output, but the project rule on owner dumps is that nothing derived leaks, and `ASSET_POLICY.md` says private observations stay under ignored `build/library/results/`, which this directory is not (it sits in `build/library/`, outside both `results/` and the `diag/` output directory that `diag_channels` scans). `build/` is ignored, so nothing reaches git, but stale directories accumulate with no cleanup and no mention in the output.
**Fix:** Trap the signals as well, then exit so the `EXIT` trap fires once:
```sh
trap 'rm -rf "$tmp"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP
```
Also update the `docs/testing.md` sentence to say which signals the trap now covers.

## Info

### IN-01: A failed run can leave the previous `diagnose.txt` in place, and a build failure is silent

**File:** `scripts/library.sh:37-40`, `scripts/library.sh:192`, `scripts/library.sh:202`
**Issue:** The output file is overwritten only on the success path. If the runner build fails (exit 1, message "could not build the runner"), the reducer cannot read its records (exit 1), or the ledger cannot be read (exit 1), `build/library/diag/diagnose.txt` from an earlier run, possibly for a different core revision, stays on disk. An owner who opens it after a failed run reads stale counts. The build failure also discards all compiler output (`>/dev/null 2>&1`), so the only hint is a fixed sentence.
**Fix:** `rm -f "$out/diagnose.txt"` right after `mkdir -p "$out"` for the diagnose branch. For the build, keep the discard but retain the status line, or write the build output to `$tmp` and print the last few lines only when it fails (the configure step prints the build directory, which is why it is discarded, so print only after filtering).

### IN-02: Tiny or malformed images land in a different bucket than `library.sh run` puts them

**File:** `scripts/library.cmake:19-28` (diagnose branch) against `scripts/library.cmake:110-114` (run branch)
**Issue:** In the run branch a probe that exits 77 with `size=N` below 8192 is `malformed_io`. The diagnose branch maps every 77 to `other unsupported` and only `rc 1` to `malformed_io`. A 3-byte or sub-8 KiB file is therefore `unsupported` in `diagnose` and `malformed_io` in `run`, and the owner compares those two summaries. The counts do not mislead about failures (neither bucket is evaluated) but the two commands disagree on the same files.
**Fix:** Apply the same size test in the diagnose branch (`if(probe MATCHES "^size=([0-9]+)\n" AND CMAKE_MATCH_1 LESS 8192)` then `malformed_io`), or move the classification into one shared include.

### IN-03: Magic frame constants and closed vocabularies are copied in three places

**File:** `runner/diag.c:76`, `runner/diag.c:83`, `scripts/library.sh:80-83`
**Issue:** The horizon `600u` and the `COGMASTER__DIAG_FRAMES - 60u` offset are bare in `cogmaster__diag_frame`, while `diag.h` names only the 1800 and the 8. The same 600 appears in the awk (`fd + 0 <= 600`, `ff != "600+"`) and in `diag_class.h`'s bucket edge. The board, fault-kind, device and where word lists in the awk `BEGIN` block are hand copies of the C tables, and the comment admits it. If the core gains a fault kind or board, the awk rejects those rows as `malformed` (counted but not grouped) and `groups_total == fail_total` still holds, so the totals check cannot catch the drift. The `library.diagnose` main test runs the real runner, but only with the kinds the fixtures happen to produce.
**Fix:** Add `COGMASTER__DIAG_VERDICT_FRAME 600u` and `COGMASTER__DIAG_RAM_WINDOW 60u` to `diag.h` and use them. For the word lists, add one check in `tests/runner/test_runner.c` (the `diag_class` case) that iterates every `COGMASTER_FAULT_KIND_*` and `COGMASTER_FAULT_DEVICE_*` constant and prints its name, then a library test that fails when a name is absent from the awk list. That is a seed (see SD-02), not a requirement.

### IN-04: Two comments in `library.sh` describe something the code does not do

**File:** `scripts/library.sh:2`, `scripts/library.sh:12`
**Issue:** Line 2 says "never print filenames, probes or guest observations to CI/evidence" while diagnose deliberately prints guest observations (first-event frame numbers, fault kind, pc) per row to stdout, which the owner sees locally. Line 12's "Fault prevented: a diagnose call that reads the owner's local settings or an absent directory" is wrong in the first half: lines 6-10 (above the guard) read `.env.local` for diagnose exactly as for `run`, and `docs/testing.md` says so ("reads `COGMASTER_ROM_DIR` as above"). The guard prevents only the absent-directory case.
**Fix:** Line 2: "never print filenames, paths, probe values or digests; diagnose prints counts, enum words, frame numbers, fault pcs and catalog titles." Line 12: "Fault prevented: a diagnose call over an unset or absent directory reporting an empty library as zero failures."

### IN-05: Impossible-state guard in `cogmaster__diag_order` and its test at frame 4294967295

**File:** `runner/diag_class.h:22-32`, `tests/runner/test_runner.c` (the `4294967295u` rows of `diag_class`)
**Category:** necessity
**Issue:** The comment and code are written "without the wrap that f + 1 has at 4294967295", and the test checks that value, for both `cogmaster__diag_bucket` and `cogmaster__diag_order`. The state cannot occur here: the only caller (`cogmaster__diag_print`, diag.c:132-133) passes `(uint32_t)fault.frame` after a run of exactly `COGMASTER__DIAG_FRAMES` (1800) calls, so `frame` is at most 1799, and `first_nonblank` is a call number of at most 1800. The `frame < first_nonblank` form is correct and costs nothing, so this is about the comment and the extra test rows, not a bug. This rests on judgment (the helpers are `static inline` in a header and could gain a caller), so it is Info.
**Fix:** Keep the comparison, drop the wrap sentence from the comment, and trim the `4294967295u` rows unless a second caller with unbounded frames is planned.

## Seeds (test and tooling code; project rule 7)

These concern tests, test registration or tooling and do not gate the phase.

- **SD-01: Test timeouts come from one hosted run each.** `tests/runner/diagnose.cmake:95` (633 s), `tests/library/tests.cmake:303-311` (206 s, 35 s, 34 s) and `tests/library/tests.cmake:294` derive each timeout as 2 x the slowest of a single hosted run ("slowest of 1 run"). 1800-frame emulation under qemu or wasm32 varies by more than 2x between runners. A slow-lane flake would read as a failing diagnose test. Re-measure from several runs before trusting the factor, or cap the frames under an emulator.
- **SD-02: No test covers drift between the C word tables and the awk word lists** (see IN-03).
- **SD-03: No test sees scratch left outside `OUT/diag`.** `diag_channels` in `tests/library/diagnose.cmake:80-92` scans stdout, stderr and the files under `${OUT}/diag`. The scratch directory is `$root/build/library/diag.XXXXXX`, outside that tree, and the tests always reach it through a normal exit, so a signal-killed run (WR-03) is invisible to every scan. After WR-03 is fixed, a case that sends SIGINT to a running diagnose over several synthetic images and then asserts `build/library/diag.*` is gone would pin it.
- **SD-04: `tests/library/diagnose.cmake` is 301 lines of scan code against a repository-check budget** (rule 7: repository-check code under a tenth of `src/` lines from phase 3). I did not measure the ratio; if `tests/quality` has a check for it, confirm this file is inside its counted set.

---

_Reviewed: 2026-11-12_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
