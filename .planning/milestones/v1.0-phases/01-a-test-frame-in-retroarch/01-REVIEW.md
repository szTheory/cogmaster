---
phase: 01-a-test-frame-in-retroarch
reviewed: 2026-10-19T23:05:18Z
depth: standard
files_reviewed: 120
files_reviewed_list:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .github/workflows/release.yml
  - .release-please-manifest.json
  - CMakeLists.txt
  - CMakePresets.json
  - PROVENANCE.md
  - README.md
  - THIRD-PARTY-NOTICES.md
  - cmake/CogmasterTests.cmake
  - cmake/cogmaster.pc.in
  - cmake/cogmasterConfig.cmake.in
  - cmake/packaging.cmake
  - cmake/toolchains/ppc-linux-gnu.cmake
  - cmake/toolchains/wasm32-wasi.cmake
  - cmake/version.h.in
  - docs/embedding.md
  - docs/maintaining.md
  - docs/runner.md
  - docs/testing.md
  - host/palette.c
  - host/palette.h
  - include/cogmaster.h
  - libretro/CMakeLists.txt
  - libretro/cogmaster_libretro.info.in
  - libretro/libretro.c
  - pins.txt
  - release-please-config.json
  - runner/bench.c
  - runner/bench.h
  - runner/clock.c
  - runner/clock.h
  - runner/crc32.c
  - runner/crc32.h
  - runner/exit_status.h
  - runner/main.c
  - runner/png.c
  - runner/png.h
  - runner/rss.c
  - runner/rss.h
  - runner/sha256.c
  - runner/sha256.h
  - runner/stop.c
  - runner/stop.h
  - scripts/check-title.sh
  - scripts/classify-changes.sh
  - scripts/publish.sh
  - src/crc32.c
  - src/fault.c
  - src/frame.c
  - src/instance.c
  - src/internal.h
  - src/state.c
  - src/state.h
  - src/testcard.c
  - tests/abi/global_fixture.c
  - tests/abi/libc_fixture.c
  - tests/abi/nofp_fixture.c
  - tests/abi/tests.cmake
  - tests/api/v1.c
  - tests/check.h
  - tests/cmake/abi.cmake
  - tests/cmake/action_pins.cmake
  - tests/cmake/api_names.cmake
  - tests/cmake/api_version.cmake
  - tests/cmake/archive_contents.cmake
  - tests/cmake/budgets.cmake
  - tests/cmake/check_hash.cmake
  - tests/cmake/check_junit.cmake
  - tests/cmake/ci_required.cmake
  - tests/cmake/e2e_budget.cmake
  - tests/cmake/exit_table.cmake
  - tests/cmake/expect_fail.cmake
  - tests/cmake/floor.cmake
  - tests/cmake/hashes.cmake
  - tests/cmake/info_claims.cmake
  - tests/cmake/install_line.cmake
  - tests/cmake/no_float.cmake
  - tests/cmake/no_skip.cmake
  - tests/cmake/notices.cmake
  - tests/cmake/perf_compare.cmake
  - tests/cmake/pins.cmake
  - tests/cmake/single_version.cmake
  - tests/cmake/timeouts.cmake
  - tests/cmake/vendored.cmake
  - tests/consumer/CMakeLists.txt
  - tests/consumer/main.c
  - tests/consumer/main.cpp
  - tests/core/test_core.c
  - tests/core/test_state.c
  - tests/core/tests.cmake
  - tests/embed/CMakeLists.txt
  - tests/header/header.c
  - tests/header/header.cpp
  - tests/host/test_palette.c
  - tests/host/tests.cmake
  - tests/hygiene/fixtures/titles.txt
  - tests/hygiene/tests.cmake
  - tests/install/tests.cmake
  - tests/inventory/ci.txt
  - tests/inventory/hygiene.txt
  - tests/inventory/nofp.txt
  - tests/inventory/wasm32.txt
  - tests/libretro/host.c
  - tests/libretro/tests.cmake
  - tests/perf/allow.txt
  - tests/perf/tests.cmake
  - tests/policy/tests.cmake
  - tests/release/tests.cmake
  - tests/retroarch/bmp.c
  - tests/retroarch/compare.c
  - tests/retroarch/flip.cmake
  - tests/retroarch/run_retroarch.cmake
  - tests/retroarch/test.cfg.in
  - tests/runner/test_png.c
  - tests/runner/test_runner.c
  - tests/runner/tests.cmake
  - tests/testcard_pins.cmake
  - tests/tests.cmake
  - version.txt
findings:
  critical: 0
  warning: 4
  info: 8
  total: 12
status: issues_found
---

# Phase 1: Code Review Report

**Reviewed:** 2026-10-19T23:05:18Z
**Depth:** standard
**Files Reviewed:** 120
**Status:** issues_found

## Summary

The scope was the 120 files changed since `75a36f40`. These were read in full: the core (`src/`, `include/`), the
host helpers, the runner, the libretro adapter, the libretro test host, the RetroArch BMP compare, the three
workflows, `scripts/publish.sh`, `scripts/classify-changes.sh`, `scripts/check-title.sh`, the CMake build and
packaging files, `docs/embedding.md`, `docs/runner.md` and the README. The CMake `-P` test scripts, the test
inventories and the ABI fixtures had a lighter pass, focused on hash and install-line handling, as the brief set
them as lower priority.

The C code holds up under adversarial tracing:

- **Size-tag checks.** The input tag is capped at 4096 and the output write uses min(tag, ours).
- **Bounds.** The pitch cap and row-offset arithmetic stay in range. The PNG stored-block bookkeeping, SHA-256
  padding, nearest-rank percentiles and Windows QPC scaling are correct.
- **Test card.** The palette math and chart indices cover the 64 SMS colours and the 4,096 GG colours exactly once.
- **State visitor.** VALIDATE-before-APPLY is atomic.
- **Libretro.** Buffer sizing and audio-remainder arithmetic are right: 3,504,512 frames over 4,375 frames was
  verified.

No memory-safety, integer-overflow or determinism defect was found in today's code paths.

The workflows use no `${{ }}` interpolation of untrusted PR data in `run:` blocks, every action is SHA-pinned and
checkouts do not persist credentials.

Four warnings remain:

1. A latent out-of-bounds hazard in the size-tag contract, which the first appended field will trigger.
2. A rehearsal skip when the PR file listing fails.
3. A release workflow that GitHub has never parsed and that uses an unvalidated `concurrency.queue` key.
4. The release App's private key is reachable from same-repository PR branches.

Under project rule 7, findings about tooling and test code (WR-02, WR-03, WR-04 and most Info items) are seeds,
not requirements.

## Narrative Findings (AI reviewer)

## Warnings

### WR-01: Size-tag contract promises zero-fill for short structs, but the core reads and writes caller structs directly

**File:** `src/internal.h:158-168`, `src/instance.c:18-33,114-118`, `src/frame.c:69-94`
**Issue:** `docs/embedding.md:57-58` says that for an input struct whose size lies between the first released size
and the library's own, "the fields the caller's header lacks read as zero". `cogmaster__check_size` only validates
the tag. Every caller then dereferences the caller's object at the library's own layout: `media->image`,
`media->machine`, `io->flags`, `io->pitch_bytes`. `cogmaster_get_frame` also writes `io->x` through
`io->backdrop_rgb444` directly.

Today first size == own size, so the gap is vacuous. The first field appended to `cogmaster_media` or
`cogmaster_frame` turns every older-header caller into an out-of-bounds read, and for `cogmaster_frame` an
out-of-bounds write. No helper exists that would make the correct pattern the default.

**Fix:** Add the helper now, while it is free, and route input structs through a zeroed local copy:
```c
/* Copies min(tag, ours) bytes of the caller's struct into a zeroed local. */
static void cogmaster__read_in(void *local, const void *in, uint32_t ours)
{
    uint32_t size = size_tag(in);
    memset(local, 0, ours);
    memcpy(local, in, size < ours ? size : ours);
}
```
In `cogmaster_get_frame`, build the output fields in the local and finish with
`cogmaster__write_out(io, &local, sizeof local)` instead of storing through `io->...`.

### WR-02: A failed PR file listing yields `release_inputs=false`, so a release-pipeline change can merge unrehearsed

**File:** `scripts/classify-changes.sh:24-44`, `.github/workflows/ci.yml:76-78`, `tests/hygiene/tests.cmake:166`
**Issue:** When `gh api .../pulls/N/files` fails, the workflow feeds the sentinel line `(listing failed)` to the
classifier. The sentinel counts as one path, so `unknown` stays false and the output is `heavy=true
release_inputs=false`. This was reproduced with `echo '(listing failed)' | sh scripts/classify-changes.sh 5`.

The same happens when a failure occurs after earlier pages were listed. A PR that changes `release.yml` or
`publish.sh` therefore skips the rehearsal whenever the listing API fails. That contradicts the script's own
contract ("a change that cannot be seen rehearses") and the test comment ("release_inputs=true ... for any list that
cannot be trusted"). The `ci.classify` case at line 166 pins the wrong behaviour.

**Fix:** Treat the sentinel as unknown in the script, and flip the test expectation to `check true true '(listing
failed)\n'`:
```sh
  case "$path" in
    '(listing failed)') unknown=true ;;
  esac
```

### WR-03: `release.yml` has never been parsed by GitHub, and nothing validates its `concurrency.queue` key

**File:** `.github/workflows/release.yml:18-20`
**Issue:** `release.yml` runs only on pushes to `main`, and it is not yet on the default branch:
`gh api .../actions/workflows` lists only CI and Nightly. GitHub has therefore never validated it. It uses
`concurrency: { group: ..., queue: max }`. I could not confirm that `queue` is a supported concurrency key. If GitHub
rejects it, the file is an "Invalid workflow file" and the first release silently never fires.

The CI rehearsal exercises `publish.sh` steps from `ci.yml`, not `release.yml` itself. No hygiene check (for example
actionlint against the current schema) parses the workflow files.

**Fix:** Before merge, confirm `queue` against the current GitHub workflow-syntax reference. If it is unsupported,
drop it: a `group` without `cancel-in-progress` still serialises runs, at the cost of a pending run being replaced.
Add a hygiene test that runs a pinned actionlint over `.github/workflows/*.yml`.

### WR-04: The release App's private key is usable from any same-repository PR branch

**File:** `.github/workflows/ci.yml:471-552`, `.github/workflows/release.yml:48-55`
**Issue:** The rehearsal job runs PR-controlled code (`scripts/publish.sh` and the rest of the merge commit) after
minting a `contents: write` App token. `COGMASTER_APP_PRIVATE_KEY` is a plain repository secret. Any workflow on any
pushed branch can read it: a branch that edits `ci.yml` can print or use the key and mint tokens with every
permission the App holds (contents, pull-requests and issues write). The App is also the identity that auto-merges
release PRs.

The rehearsal design needs the key on PR runs, but nothing narrows where it can be used.
**Fix:** Move the key into a GitHub Environment (for example `release`) with required reviewers or a deployment
branch policy. Reference that environment only from `release.yml`'s jobs and from a rehearsal job gated on it.
Alternatively, give the rehearsal its own App, whose installation holds only `contents: write` on this repository.

## Info

### IN-01: The comment and docs say the clock is 10,738,635 Hz, but the frame-rate fraction is for 10,738,636.36 Hz

**File:** `src/instance.c:160-162`, `docs/embedding.md:129`
**Issue:** 10,738,635 / 179,208 reduces to 3,579,545 / 59,736. The code returns 1,640,625 / 27,379, which is
(315 MHz / 88 × 3) / 179,208, the exact NTSC master clock of 118,125,000 / 11 Hz. The value is right; the cited
number is not.
**Fix:** Say "315/88 MHz × 3 (about 10,738,636 Hz)" in both places.

### IN-02: The PNG zlib header comment states the wrong window size

**File:** `runner/png.c:111-113`
**Issue:** CMF 0x78 is CINFO 7, a 32 KiB window, not "a 256-byte window". The bytes themselves are valid: 0x7801 is a
multiple of 31.
**Fix:** Correct the comment.

### IN-03: The nightly comment claims the default shell has no pipefail

**File:** `.github/workflows/nightly.yml:101-102`
**Issue:** `defaults.run.shell: bash` makes GitHub run `bash --noprofile --norc -eo pipefail {0}`, so pipefail is on.
The comment misleads the next editor.
**Fix:** Reword the comment, or drop the claim.

### IN-04: `publish.sh prepare` matches asset names as regular expressions

**File:** `scripts/publish.sh:45`
**Issue:** `names | grep -qx "$n"` treats the `.` in `cogmaster-*.zip` as a wildcard, so an unexpected file such as
`cogmaster-runner-linux-x64Xzip` is not reported as unexpected.
**Fix:** Use `grep -qxF -- "$n"`.

### IN-05: The runner's and the adapter's frame-size guards run after the core has already written the buffer

**File:** `runner/main.c:242-253`, `libretro/libretro.c:248-252`
**Issue:** The `io.w > 256` and `io.w > info.max_w` checks happen after `cogmaster_get_frame` has filled the buffer,
so they cannot prevent an overflow. The runner also never checks `io.h`, and its 256 × 240 buffer is hard-coded
rather than sized from `cogmaster_get_info`. Both are safe today only by the core's contract.
**Fix:** Before the call, check that `info.crop_w * info.crop_h` (or `max_w * max_h`) fits the buffer, and size the
runner's buffer from `info`.

### IN-06: The CRC-32 table is duplicated in the core and in the runner

**File:** `src/crc32.c:7-44`, `runner/crc32.c:5-42`
**Issue:** The 256-entry table appears twice. The runner cannot reach the core's internal symbol, so a divergence
would go unnoticed except through the PNG tests.
**Fix:** Accept this as deliberate (the runner uses only the public API) and say so in one comment, or have a test
compare the two functions on random inputs.

### IN-07: The state checksum leaves the header's `frame` and `ticks` unprotected, and the fault frame is not cross-checked

**File:** `src/state.c:64-69,97-109,130-133`
**Issue:**
- The body CRC starts at byte 64, so `frame` and `ticks` are guarded only by `ticks == frame * 179208`. A corruption
  that keeps that relation, or that hits a field whose valid range is the full width, loads silently.
- `fault_frame > frame` is also accepted.

**Fix:** Before the format is frozen by a public state API:
- Extend the CRC over the header, with the CRC field treated as zero.
- Add `cogmaster__sio_check(io, !present || fault_frame <= frame)`. This needs the frame passed into `fault_io`.

### IN-08: Rehearsals publish real attestations for PR-built archives on the public repository

**File:** `.github/workflows/ci.yml:537-552`
**Issue:** Each rehearsal attests PR-built zips under this repository's identity. `publish.sh verify` is strict
about the signer and source ref. A user running `gh attestation verify --repo szTheory/cogmaster` without
`--signer-workflow` or `--source-ref` would accept a rehearsal artifact, and the rehearsal release itself is
deleted afterwards.
**Fix:** In `docs/maintaining.md`, or wherever verification is documented for users, give the full verify command
with `--signer-workflow szTheory/cogmaster/.github/workflows/release.yml --source-ref refs/heads/main`.

---

_Reviewed: 2026-10-19T23:05:18Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
