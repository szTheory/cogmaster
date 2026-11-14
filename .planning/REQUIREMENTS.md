# Requirements: cogmaster v2.2 Legacy video modes

**Defined:** 2026-11-13
**Core Value:** Master System and Game Gear games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.

## v2.2 Requirements

Requirements for this milestone. Each maps to one roadmap phase. Scope approved by the owner on 2026-11-13 after a nine-lens
fan-out and an adversarial pass (`.planning/research/NEXT-MILESTONE.md`): Graphics I and II only, the SG-1000 machine, text and
multicolour modes and states are later. The 46 default-profile `fail` rows that wait on legacy modes are an upper bound, not
a promise: a row also needs a changing picture to pass the boots check, and some rows land on a later fault.

### Legacy video modes

- [x] **MODE-01**: With register 0 and 1 selecting Graphics I (M1=0, M2=0, M3=0, M4=0), the core draws the name table, the pattern table, the colour table (one byte per eight patterns) and the register 7 backdrop with the fixed TMS9918 palette, on the Master System 1, Master System 2 and Game Gear in Master System video mode; an owned fixture proves the pixels against a closed-form oracle written before the renderer
- [x] **MODE-02**: With Graphics II selected (M3=1), the core draws the three-section pattern and colour tables with the documented address masks (registers 3 and 4); a fixture with each mask combination proves the pixels, and the masks that the sources disagree on are named in the hardware notes with the choice and its reason
- [x] **MODE-03**: Sprites in the legacy modes follow the documented legacy rules (4 per line, 8x8 and 16x16, magnification, early clock bit, fifth-sprite flag, coincidence flag, the end-of-list Y value), proven by a fixture with closed-form pixels and status-register checks, and the Mode 4 sprite engine is untouched
- [x] **MODE-04**: Text, multicolour, the mixed modes, the invalid text modes and native Game Gear video keep recording the unsupported-video-mode fault, each with a test, and the documentation says which modes render and which fault; the Light Phaser's "would this fault" check agrees with the supported-mode test
- [x] **MODE-05**: No shipped frame, audio or state pin moves (the base-versus-head hashes comparison from the previous milestone is equal for every pin), behaviour revision 19 and state format 17 are recorded, a legacy-mode state round trip is equal, and the state loader refuses a state whose legacy sprite slots break the new bounds
- [x] **MODE-06**: Mode switches during a run draw correctly: a fixture that enters Graphics II after frame 600 and one that switches modes mid-frame each have a pinned hash, and the mode 4 bring-up dots that wrote registers 0 and 1 transiently still hash as before
- [x] **MODE-07**: The legacy renderer is selected once per line, and the instructions-per-frame gate stays green with no new allow line; the first hosted read of the gate comes from a draft pull request opened before the phase is complete
- [x] **MODE-08**: The hardware notes on the legacy modes are rewritten from sources (the earlier note that MAME contradicts is corrected), the permissive reference files consulted have rows in the provenance document, and the docs stay inside their byte caps

### Library run

- [ ] **RUN-03**: After the phase 16 release merges, the library judge is re-run on `main` over the owner's library, so the ledger stamp names a commit that is an ancestor of `main`, and the rows that left `fail` are merged by pull request through the unchanged ratchet with no `pass` row lost
- [ ] **RUN-04**: The compatibility page and its counts equal its regeneration from the merged ledger, `compat.exit` stays green, and the rows that still fail are counted by cause in the diagnose

### Debt and tooling

- [x] **TUNE-12**: The documentation states the current fault vocabulary: the false line in `docs/audio.md` about unmapped-port faults and the stale lines in `docs/embedding.md` and `docs/runner.md` are corrected, and the docs checks pass
- [x] **TUNE-13**: The library run path refuses to stamp `core=` from a tree with tracked changes outside `.planning`, or stamps `+dirty` and the ratchet rejects that stamp; a test with a dirty-tree control proves it
- [x] **TUNE-14**: A transient DNS or TLS failure while fetching a pinned assembler or tool no longer fails a hosted job on its first attempt; the fetch script retries those errors, with a test that a permanent failure still fails
- [ ] **TUNE-15**: The diagnose vocabulary that nothing raises any more (`pclass`, `wclass`, `where=port`, `rom_write`, `vdpmode`) is removed in one change, the line shape changes once, and the diagnose tests and docs agree
- [ ] **TUNE-16**: The job timeouts that drifted again (the be32 lane, the diagnose test, the hygiene tree check) are re-derived from live runs by the twice-the-slowest rule, with run IDs, in one workflow commit
- [ ] **TUNE-17**: A milestone-close check script reports open pull requests, main CI on the latest commit, the latest release and its release pull request, and a clean work tree on `main`, exits non-zero when any is off, is described in `docs/maintaining.md`, and has a test with a dirty-tree control; it is a check to run, not a release gate

## Future Requirements

Deferred, tracked but not in the current roadmap.

### Ledger and judges

- **LEDG-06**: A catalog `video` column for the 50 Game Gear entries MAME runs in Master System mode
- **LEDG-07**: Per-alias `supported` and a reason for `partial` entries
- **PAD-08**: The pad, Light Phaser and BIOS judges and the scan thresholds on the owner's hardware

### Video

- **MODE-09**: Text and multicolour modes, only if the diagnose shows 5 or more rows landing on them
- **SG-01**: The SG-1000 and SG-1000 II machine as a thin profile (the next machine milestone)

### Hosts

- **STATE-01**: A public state API, libretro save states, rewind, run-ahead and a `SYSTEM_RAM` export (v3.0)

## Out of Scope

| Feature | Reason |
|---------|--------|
| The SG-1000 machine, the SC-3000 keyboard and tablet, multicarts | Separate risk and a weaker pass predicate; the machine is v3.1 and the rest has no evidence |
| Save states, rewind, run-ahead, movies | v3.0; the state layout changes here, so states follow once it settles |
| The browser build | Settled as DEC.68: the library as wasm here and the page in a sibling repository, after v3.0 |
| A MAME screenshot as a gate | The reference run needs firmware bytes the agent may not hold and `mame` is not installed here; a by-eye comparison is optional |
| A standalone tune-up phase | The debt is small and lands in files this feature edits |
| Python or npm packages, a libretro buildbot listing | Supply-chain surface, and the listing waits for save states and the owner |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| MODE-01 | Phase 16 | Complete |
| MODE-02 | Phase 16 | Complete |
| MODE-03 | Phase 16 | Complete |
| MODE-04 | Phase 16 | Complete |
| MODE-05 | Phase 16 | Complete |
| MODE-06 | Phase 16 | Complete |
| MODE-07 | Phase 16 | Complete |
| MODE-08 | Phase 16 | Complete |
| RUN-03 | Phase 17 | Pending |
| RUN-04 | Phase 17 | Pending |
| TUNE-12 | Phase 16 | Complete |
| TUNE-13 | Phase 16 | Complete |
| TUNE-14 | Phase 16 | Complete |
| TUNE-15 | Phase 17 | Pending |
| TUNE-16 | Phase 17 | Pending |
| TUNE-17 | Phase 17 | Pending |

**Coverage:**
- v2.2 requirements: 16 total
- Mapped to phases: 16 (Phase 16: 11, Phase 17: 5)
- Unmapped: 0 ✓

---
*Requirements defined: 2026-11-13*
*Last updated: 2026-11-13 after roadmap creation (phases 16 and 17)*
