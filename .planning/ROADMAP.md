# Roadmap: cogmaster

## Milestones

- ✅ **v1.0 SMS and Game Gear games play with sound in RetroArch** — Phases 1-4 (shipped 2026-11-09, releases v0.1.0 to v0.4.0)
- ✅ **v2.0 The whole library** — Phases 5-12 (shipped 2026-11-12, releases v0.5.0 to v0.12.0)
- 🚧 **v2.1 Fewer failing cartridges** — Phases 13-15 (in progress)

## Phases

<details>
<summary>✅ v1.0 SMS and Game Gear games play with sound in RetroArch (Phases 1-4) — SHIPPED 2026-11-09</summary>

- [x] Phase 1: A test frame in RetroArch (12/12 plans) — completed 2026-10-19
- [x] Phase 2: The CPU matches the public vectors (17/17 plans) — completed 2026-10-20
- [x] Phase 3: Games play in RetroArch (18/18 plans) — completed 2026-10-20
- [x] Phase 4: Sound (15/15 plans) — completed 2026-11-09

Full phase details: `.planning/milestones/v1.0-ROADMAP.md`. Requirements: `.planning/milestones/v1.0-REQUIREMENTS.md`. Audit: `.planning/milestones/v1.0-MILESTONE-AUDIT.md`.

</details>

<details>
<summary>✅ v2.0 The whole library (Phases 5-12) — SHIPPED 2026-11-12</summary>

- [x] Phase 5: Tune-up (14/14 plans) — completed 2026-11-10
- [x] Phase 6: Korean cartridges play (11/11 plans) — completed 2026-11-10
- [x] Phase 7: Saves survive a restart (10/10 plans) — completed 2026-11-10
- [x] Phase 8: Sports Pad, Paddle and 3-D glasses (17/17 plans) — completed 2026-11-11
- [x] Phase 9: Light Phaser (12/12 plans) — completed 2026-11-11
- [x] Phase 10: BIOS boot (11/11 plans) — completed 2026-11-11
- [x] Phase 11: PAL and Japanese FM (9/9 plans) — completed 2026-11-11
- [x] Phase 12: The whole list in the ledger (10/10 plans) — completed 2026-11-12

Full phase details: `.planning/milestones/v2.0-ROADMAP.md`. Requirements: `.planning/milestones/v2.0-REQUIREMENTS.md`. Audit: `.planning/milestones/v2.0-MILESTONE-AUDIT.md`.

</details>

### 🚧 v2.1 Fewer failing cartridges (In Progress)

**Milestone goal:** find out why a third of the checked cartridges fail the boots check, fix the causes the evidence names, and merge the rows that flip to `pass`. The milestone is SEED-016 plus the CI timeout drift from SEED-015. No phase sets a target number of flipped rows: the causes decide the count.

- [ ] **Phase 13: Why carts fail** - A counts-only diagnostic in the library judge, run over the owner's library, grouped by cause
- [ ] **Phase 14: Fix the causes** - Each cause group of 5 or more rows fixed in the core or the boots check, or closed with a stated reason
- [ ] **Phase 15: Re-run, merge and timeouts** - Flipped rows merged through the ratchet, the page agrees, and the drifted CI timeouts re-derived

## Phase Details

### Phase 13: Why carts fail
**Goal**: The failing default-profile rows have a known cause, grouped by size, from a diagnostic that leaks nothing from the owner's dumps. No fix is made in this phase and no ledger row moves.
**Depends on**: Nothing (first phase of v2.1; v2.0 is shipped as v0.13.0)
**Requirements**: DIAG-01, DIAG-02, DIAG-03
**Success Criteria** (what must be TRUE):
  1. Running the library judge in diagnostic mode prints, for each default-profile `fail` row, the frames to the first non-blank sample, the first fault kind and PC, and the mapper chosen, and nothing but counts, kinds and catalog titles
  2. A test over synthetic fixtures shows the diagnostic output holds no image bytes and no file names or paths, and a negative control that leaks one makes the same test fail
  3. A planning note records the failing rows grouped by cause with a count per group, counts only and no dump content, ordered by descending group size, so the next phase can cite it
  4. `cmake --workflow --preset ci` passes and the committed ledger and compatibility page are unchanged
**Plans**: 4/5 plans executed
Plans:
**Wave 1**
- [x] 13-01-PLAN.md — Runner `--diagnose` mode: one closed line per cart, frame-600 verdict shared with `--expect boots@600`

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 13-02-PLAN.md — `sh scripts/library.sh diagnose`: parse-and-reconstruct validator, fixed cause leaves, counter totals, ledger and page unchanged

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 13-03-PLAN.md — Diagnose output completed: sub-rows and group order, cross-cuts, pass histogram, ledger cross-check, canned-line leaf coverage

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 13-04-PLAN.md — Leak test: canary scan over every channel, negative controls, hostile inputs, docs

**Wave 5** *(blocked on Wave 4 completion)*
- [ ] 13-05-PLAN.md — Run over the owner's library, the counts-only cause-groups note, timeouts settled from a hosted run
**Note**: The owner passes `COGMASTER_ROM_DIR` in the shell where the agent runs the diagnostic; the agent reads counts, kinds and catalog titles only and commits nothing from the dumps.

### Phase 14: Fix the causes
**Goal**: Every cause group of 5 or more failing rows is fixed in the core or the boots check, or closed with a stated reason, so carts that run correctly stop failing for the wrong reason. The fixes cannot be planned in detail until Phase 13's grouping exists; the plans are derived from that note, and no cause is assumed here.
**Depends on**: Phase 13
**Requirements**: FIX-01, FIX-02, FIX-03, FIX-04
**Success Criteria** (what must be TRUE):
  1. Each cause group of 5 or more rows in the Phase 13 note has a recorded outcome, fixed or closed with a stated reason, worked in descending order of group size and citing the note
  2. If the diagnostic shows the boots check rejects carts that run correctly, the check is corrected and a fixture shows a late-first-picture cart passes while a cart that never draws still fails; if it shows the check is sound, that is the stated reason and the existing never-draws fixture stays
  3. The `zemina`, `korean_nb` and `korean` fail rows each have a stated cause and are fixed or refused with a closed reason
  4. Every core fix has a test that `cmake --workflow --preset ci` runs, and any shipped frame or audio pin that moves is named, justified and recorded with a behaviour revision bump
  5. `cmake --workflow --preset ci` passes on the phase head and no existing `pass` row's fixture result changes without a named pin
**Plans**: TBD

### Phase 15: Re-run, merge and timeouts
**Goal**: The rows the fixes flipped to `pass` are in the ledger and the compatibility page, and the CI job timeouts that drifted from the twice-the-slowest rule are re-derived from live runs.
**Depends on**: Phase 14
**Requirements**: RUN-01, RUN-02, TUNE-11
**Success Criteria** (what must be TRUE):
  1. The library judge re-run over the owner's library after the fixes yields a ledger update that merges by pull request through the existing ratchet, and no `pass` row becomes `fail`
  2. The compatibility page and its counts agree with the merged ledger, and `compat.exit` stays green
  3. The asan, nightly bench and bench-wasm32 job timeouts and the fuzz and digest comments are re-derived by the twice-the-slowest rule from live runs, each timeout with its run IDs, in one edit per workflow file, and `hygiene.timeout_comments` passes
  4. `cmake --workflow --preset ci` passes and the pull request has a CI run on its head commit
**Plans**: TBD
**Note**: The owner passes `COGMASTER_ROM_DIR` for the re-run; nothing from the dumps is committed. `ci.yml` is edited once in this phase, in the last push, and no earlier v2.1 phase edits it.

## Progress

| Milestone | Phases | Plans | Status | Shipped |
|-----------|--------|-------|--------|---------|
| v1.0 | 4/4 | 62/62 | Complete | 2026-11-09 |
| v2.0 | 8/8 | 94/94 | Complete | 2026-11-12 |
| v2.1 | 0/3 | 0/? | In progress | - |

v2.1 phases: 13 Why carts fail (0/5 plans, not started), 14 Fix the causes (0/? plans, not started), 15 Re-run, merge and timeouts (0/? plans, not started).
