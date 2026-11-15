# Phase 16 code review disposition

Review: 16-REVIEW.md (standard depth, 46 files: 0 critical, 3 warning, 4 info). No fix round has run.

| ID | Severity | Disposition | Note |
|----|----------|-------------|------|
| WR-01 | warning | open | Comment in src/internal.h and src/state.c wrongly says the cram and vram offsets stay put; each sprite slot gained an attr byte, so both move by 8. No code reads them. |
| WR-02 | warning | open | Legacy overflow record only checks overflow <= 32, count == 4, scanned >= 5; a real scan keeps overflow == scanned. Memory-safe, weaker than its stated contract. |
| WR-03 | warning | open | Download seed: no TIMEOUT or INACTIVITY_TIMEOUT in WlaDx.cmake, no curl --connect-timeout or --max-time in fetch-pin.sh. Tooling code, so a seed under rule 7. |
| IN-01 | info | open | cogmaster__vdp_background is only called from a test now; the display-off branch in cogmaster__legacy_span is unreachable. |
| IN-02 | info | open | Status bit masks and the nothing-latched value 31 repeat across four files. |
| IN-03 | info | open | Seed: library.sh treats any git failure as a dirty tree; docs/testing.md omits that the refusal applies only to the tracked ledger. |
| IN-04 | info | open | Seed: stale provisional timeout comments in tests/policy/tooling_cases.cmake and tests/vdp/tests.cmake. |
