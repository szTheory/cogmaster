# Phase 16: deferred items

Out-of-scope discoveries logged by plan executors. Nothing here blocks a plan; none was fixed in the plan that found it.

## 16-01

- `hygiene.tree` (timeout 60 s) passes alone in about 43 s on the local machine but exceeded 60 s when the whole hygiene
  lane ran in parallel (`ctest --preset hygiene`, 84 tests, 18 cores). The rerun of that one test passed. The plan's files
  do not touch the scan; the cost is the scan itself. A timeout re-derivation from a hosted run belongs to the CI-speed
  seeds, not to this phase.

## 16-03

- `hygiene.tree` took 58.1 s of its 60 s timeout inside `cmake --workflow --preset hygiene` (84 tests in parallel) and
  passed; the hosted hygiene job passed on the same head. Same item as 16-01, nearer the limit; nothing in this plan's files
  touches the scan.

## 16-04

- `release.firmware_scan` (step 5 of `cmake --workflow --preset ci`) read the macOS arm64 `lib/libcogmaster.a` as a
  copier-headered firmware image when the archive was exactly 262656 bytes (256 KiB plus 512), one of the five sizes in
  `tests/cmake/firmware_scan.cmake`. The renderer commit landed on that size by chance. The cause is a size rule applied
  to a static library; this plan moved the archive to 262024 bytes with the per-cell colour refactor and left the scan
  alone. Any later commit can land a platform's archive on one of the five sizes again, so a hosted build job may show
  this failure on a push; the fix belongs to the scan (skip a file that starts with the `ar` magic), not to the code.
- `hygiene.tree` exceeded its 60 s limit when the machine was loaded by unrelated processes (load average 30 to 40): the
  scan itself passes, exit 0, in 69 s with `sh scripts/hygiene.sh --tree`. `cmake --workflow --preset hygiene` exits 0,
  84 of 84, with `COGMASTER_TIMEOUT_SCALE=4` in `build/hygiene` (reset to 1 afterwards). Same item as 16-01 and 16-03.
