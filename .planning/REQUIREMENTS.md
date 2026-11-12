# Requirements: cogmaster v2.1 Fewer failing cartridges

**Defined:** 2026-11-12
**Core Value:** Master System and Game Gear games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.

## v2.1 Requirements

Requirements for this milestone. Each maps to one roadmap phase. Scope decisions made with the owner on 2026-11-12: the milestone is SEED-016 (the failing carts) plus the SEED-015 timeout drift; the agent runs the library diagnostic in a shell where the owner passes `COGMASTER_ROM_DIR`, reads counts, kinds and catalog titles only, and commits nothing from the dumps; no research phase. No requirement sets a target number of flipped rows: the count is whatever the causes explain.

### Diagnostic

- [x] **DIAG-01**: The library judge has a diagnostic mode that, for each default-profile `fail` row, reports the frames to the first non-blank sample, the first fault kind and PC, and the mapper chosen, and prints only counts, kinds and catalog titles, never ROM bytes, file names or paths
- [x] **DIAG-02**: A test with synthetic fixtures proves the diagnostic output holds no image bytes or file names, and a negative control that leaks one makes the test fail
- [ ] **DIAG-03**: The failing rows are grouped by cause with a count per group, and the grouping (counts only, no dump content) is recorded in a planning note that the fixes cite

### Fixes

- [ ] **FIX-01**: Each cause group of 5 or more rows is either fixed in the core or the boots check, or closed with a stated reason, in descending order of group size
- [ ] **FIX-02**: If the boots check rejects carts that run correctly, the check is corrected, and a fixture shows that a late-first-picture cart passes while a cart that never draws still fails
- [ ] **FIX-03**: The `zemina`, `korean_nb` and `korean` fail rows each have a stated cause and are fixed or refused with a closed reason
- [ ] **FIX-04**: Every core fix lands with a test in the `ci` workflow, and any shipped frame or audio pin that moves is named, justified and recorded with a behaviour revision bump

### Library run

- [ ] **RUN-01**: The library judge is re-run over the owner's library after the fixes, and the rows that flipped to `pass` are merged by pull request through the existing ledger ratchet, with no `pass` row becoming `fail`
- [ ] **RUN-02**: The compatibility page and its counts agree with the merged ledger, and `compat.exit` stays green

### CI timeouts

- [ ] **TUNE-11**: The asan, nightly bench and bench-wasm32 job timeouts and the fuzz and digest comments are re-derived from live runs by the twice-the-slowest rule, with run IDs, in one edit per workflow file

## v2.2 Requirements

Deferred to a later milestone. Tracked but not in this roadmap.

### Ledger rows (SEED-015)

- **LEDG-06**: A catalog `video` column for the 50 Game Gear entries that MAME runs in Master System mode
- **LEDG-07**: Per-alias `supported`, and a reason for `partial` entries

### Later horizons

- **PAD-08**: The pad, Light Phaser and BIOS judges (SEED-010) and the board-scan thresholds (SEED-009) on the owner's hardware

## Out of Scope

| Feature | Reason |
|---------|--------|
| PAL and Japanese FM `unknown` rows for every Master System identity | No run stands behind them; the owner's runs add the profiles a real cart reaches |
| New cartridge boards beyond what a failing group names | Fix what the diagnostic shows, not what is guessed |
| A numeric target for flipped rows | The causes decide the count; a target would invite loosening the check |
| Reading or committing any dump bytes, file names or paths | `ASSET_POLICY.md`: the owner's dumps stay on the owner's machine |
| The CI nightly freshness step and the other SEED-015 `ci.yml` items | They edit `ci.yml` and wait for a phase that does not already edit it |
| States and movies, measured accuracy, tools, ports, the browser, SG-1000 | Later milestones, seeds and horizons in `.planning/PRODUCT-ROADMAP.md` |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| DIAG-01 | Phase 13 | Complete |
| DIAG-02 | Phase 13 | Complete |
| DIAG-03 | Phase 13 | Pending |
| FIX-01 | Phase 14 | Pending |
| FIX-02 | Phase 14 | Pending |
| FIX-03 | Phase 14 | Pending |
| FIX-04 | Phase 14 | Pending |
| RUN-01 | Phase 15 | Pending |
| RUN-02 | Phase 15 | Pending |
| TUNE-11 | Phase 15 | Pending |

**Coverage:**
- v2.1 requirements: 10 total
- Mapped to phases: 10 (Phase 13: 3, Phase 14: 4, Phase 15: 3)
- Unmapped: 0

---
*Requirements defined: 2026-11-12*
*Last updated: 2026-11-12 after roadmap creation (Phases 13 to 15)*
