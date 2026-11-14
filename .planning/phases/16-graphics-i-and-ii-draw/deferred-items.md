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
