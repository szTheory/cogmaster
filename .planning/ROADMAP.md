# Roadmap: cogmaster

## Milestones

- ✅ **v1.0 SMS and Game Gear games play with sound in RetroArch** — Phases 1-4 (shipped 2026-11-09, releases v0.1.0 to v0.4.0)
- ✅ **v2.0 The whole library** — Phases 5-12 (shipped 2026-11-12, releases v0.5.0 to v0.13.0)
- ✅ **v2.1 Fewer failing cartridges** — Phases 13-15 (shipped 2026-11-13, releases v0.14.0 to v0.14.2)
- 🚧 **v2.2 Legacy video modes** — Phases 16-17 (in progress)

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

<details>
<summary>✅ v2.1 Fewer failing cartridges (Phases 13-15) — SHIPPED 2026-11-13</summary>

- [x] Phase 13: Why carts fail (5/5 plans) — completed 2026-11-12
- [x] Phase 14: Fix the causes (8/8 plans) — completed 2026-11-13
- [x] Phase 15: Re-run, merge and timeouts (2/2 plans) — completed 2026-11-13

Full phase details: `.planning/milestones/v2.1-ROADMAP.md`. Requirements: `.planning/milestones/v2.1-REQUIREMENTS.md`. Audit: `.planning/milestones/v2.1-MILESTONE-AUDIT.md`.

</details>

### 🚧 v2.2 Legacy video modes (In progress)

**Milestone goal:** cartridges that use the TMS9918 Graphics I and II modes draw their real picture instead of faulting, so
most of the 46 default-profile `fail` rows that wait on these modes leave the unsupported-mode leaf. The 46 is an upper
bound, not a target: a row also needs a changing picture to pass the boots check, and some rows will land on a later
fault. Acceptance rests on owned fixtures and closed-form oracles, not on the flipped-row count. Two phases, each ending
in a release; the scope and its reasoning are in `.planning/research/NEXT-MILESTONE.md`.

- [x] **Phase 16: Graphics I and II draw** - The legacy renderer and its sprites on the Master System video profiles, with every shipped pin unchanged; the doc and tooling riders land as the first plan (release type `feat`) (completed 2026-11-14)
- [ ] **Phase 17: Re-run, merge and clean state** - The library re-run on `main` merged through the ratchet, the dead diagnose vocabulary pruned, timeouts re-derived and a milestone-close clean-state check (release type `fix(compat)`)

## Phase Details

### Phase 16: Graphics I and II draw

**Goal**: With Graphics I or Graphics II selected, the Master System 1, Master System 2 and Game Gear in Master System
video mode draw the name, pattern and colour tables, the backdrop and the legacy sprites as the TMS9918 does, instead of
recording the unsupported-video-mode fault; every other legacy mode keeps its fault with the reason stated, no shipped
frame, audio or state pin moves, and the per-frame instruction gate stays green. The stale fault-vocabulary doc lines,
the dirty-tree stamp and the assembler fetch retry land first, inside this phase, so they ride a release.
**Depends on**: Nothing (first phase of v2.2; v2.1 is shipped as v0.14.2)
**Requirements**: TUNE-12, TUNE-13, TUNE-14, MODE-01, MODE-02, MODE-03, MODE-04, MODE-05, MODE-06, MODE-07, MODE-08
**Success Criteria** (what must be TRUE):
  1. Owned Graphics I and Graphics II fixtures (one per register 3 and 4 mask combination) and a legacy-sprite fixture run through `cogmaster-run` on the Master System 1, Master System 2 and Game Gear-in-Master-System-mode profiles, and their pixels equal closed-form oracles committed before the renderer; the sprite fixture's status-register reads (fifth-sprite and coincidence flags, the end-of-list Y value, 8x8 and 16x16, magnification, early clock) equal the oracle too
  2. Text, multicolour, the mixed modes, the invalid text modes and native Game Gear video each still record the unsupported-video-mode fault in a test, the Light Phaser's "would this fault" check agrees with the supported-mode test, and the video documentation names which modes render and which fault
  3. The base-versus-head hashes comparison is equal for every shipped frame, audio and state pin; behaviour revision 19 and state format 17 are recorded; a legacy-mode state round trip is byte-equal and a state whose legacy sprite slots break the new bounds is refused; the fixture that enters Graphics II after frame 600 and the mid-frame mode-switch fixture each have a pinned hash, and the mode 4 bring-up fixtures hash as before
  4. The instructions-per-frame gate reports green with no new allow line, first read from a draft pull request opened before the phase is complete; the legacy-mode hardware notes are rewritten from sources with the note that MAME contradicts corrected and the open model choices (Graphics II masks, sprite Y threshold, colour-0 collision) stated with their reasons, the permissive files consulted have provenance rows, and the docs byte-cap check passes
  5. The docs state the current fault vocabulary (`docs/audio.md`, `docs/embedding.md` and `docs/runner.md` corrected) and the docs checks pass; the library run path refuses or marks a tree with tracked changes outside `.planning`, proven by a test with a dirty-tree control; the tool fetch script retries a transient DNS or TLS error and a test shows a permanent failure still fails; `cmake --workflow --preset ci` exits 0 on the phase head
**Plans**: 9/9 plans complete

Plans:
**Wave 1**
- [x] 16-01-PLAN.md — Riders: fault-vocabulary doc fixes, dirty-tree refusal on the library run, transient-error retry for the assembler and tool fetches (wave 1)

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 16-02-PLAN.md — Base hashes, six owned legacy images and the closed-form and in-process oracles, before any renderer (wave 2)

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 16-03-PLAN.md — Once-per-call class dispatch skeleton, faulting-mode seeds retargeted to text and multicolour, draft pull request and first perf read (wave 3)

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 16-04-PLAN.md — Graphics I and II renderer with the fault narrowed, the unsupported-mode literal table and the in-process background, switch and hostile cases (wave 4)

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 16-05-PLAN.md — legacy_modes fixtures against the oracle on three profiles, switch-line pin and cross-lane hash rows from a frame-count parameter (wave 5)

**Wave 6** *(blocked on Wave 5 completion)*
- [x] 16-06-PLAN.md — State format 17 and behaviour revision 19: widened sprite slots, legacy round trip and loader bounds (wave 6)

**Wave 7** *(blocked on Wave 6 completion)*
- [x] 16-07-PLAN.md — Legacy sprite engine: scan, pixel, fifth-sprite and coincidence flags, class-change rescan (wave 7)

**Wave 8** *(blocked on Wave 7 completion)*
- [x] 16-08-PLAN.md — legacy_sprites fixture and mailbox rows on three profiles, hash rows, hostile sprite input, fuzz seed, second push (wave 8)

**Wave 9** *(blocked on Wave 8 completion)*
- [x] 16-09-PLAN.md — Hardware notes and provenance rewritten, embedding and conformance docs, timeouts re-derived, phase-head evidence and last push (wave 9)
**Note**: The first plan carries TUNE-12, TUNE-13 and TUNE-14 (a docs- or CI-only pull request cuts no release, and every phase ends in one). Oracles and owned fixtures come before the renderer; a draft pull request opens early so the hosted perf gate is read before the renderer is final. The renderer is its own module, selected once per line, with the fixed TMS palette and a 4-slot legacy sprite engine; the mode 4 sprite engine is untouched. No MAME screenshot is a gate (no firmware bytes here and `mame` is not installed).

### Phase 17: Re-run, merge and clean state

**Goal**: The rows the legacy renderer moved are in the ledger and on the compatibility page under a core stamp that is an
ancestor of `main`, the rows that still fail are counted by cause, the diagnose vocabulary nothing raises is gone, the
drifted job timeouts are re-derived from live runs, and a maintainer can check that the repository is in a clean state
before a milestone-close pull request.
**Depends on**: Phase 16 (its release merged on `main`)
**Requirements**: RUN-03, RUN-04, TUNE-15, TUNE-16, TUNE-17
**Success Criteria** (what must be TRUE):
  1. The library judge, re-run on `main` after the Phase 16 release merges, yields a ledger whose `core=` stamp names a commit that `git merge-base --is-ancestor` accepts against `origin/main`, and the update merges by pull request through the unchanged ratchet with no `pass` row lost
  2. The compatibility page and its counts equal their regeneration from the merged ledger, `compat.exit` exits 0, and `sh scripts/library.sh diagnose` reports the rows that still fail counted by cause
  3. The diagnose vocabulary nothing raises any more is removed in one change, the diagnose line shape changes once, and the diagnose tests and `docs/runner.md` agree with the new shape
  4. The be32, diagnose-test and hygiene tree-check timeouts are re-derived by the twice-the-slowest rule from live runs, each with its run IDs, in one workflow commit, and `hygiene.timeout_comments` passes
  5. A milestone-close check script reports open pull requests, main CI on the latest commit, the latest release and its release pull request, and a clean work tree on `main`, exits non-zero when any is off (a test with a dirty-tree control proves it), and is described in `docs/maintaining.md` as a check to run, not a release gate; `cmake --workflow --preset ci` exits 0 on the phase head
**Plans:** 4 plans

Plans:
**Wave 1**
- [ ] 17-01-PLAN.md — Diagnose prune: `diagnose 2`, 30-field line, reducer, tests and docs agree; TUNE-15 wording keeps `vdpmode` (wave 1)

**Wave 2** *(blocked on Wave 1 completion)*
- [ ] 17-02-PLAN.md — Milestone-close check script with a fake-gh case loop and dirty-tree controls, maintainer docs, one live run on clean `main` (wave 2)

**Wave 3** *(blocked on Wave 2 completion)*
- [ ] 17-03-PLAN.md — Push plans 01 and 02, owner session: judge on a worktree of the phase base, carry ledger and page, diagnose counts (wave 3, owner checkpoint)

**Wave 4** *(blocked on Wave 3 completion)*
- [ ] 17-04-PLAN.md — Re-derive drifted timeouts from a 40-run read in one last workflow commit, ship hand-off (wave 4)
**Note**: RUN-03 follows Phase 15's RUN-01 pattern: the owner passes `COGMASTER_ROM_DIR` in the shell where the agent runs the judge, the agent reads counts only, and nothing from the dumps is committed. Which of `pclass`, `wclass`, `where=port`, `rom_write` and `vdpmode` are dead is confirmed at the discuss step against the Phase 16 code (text and multicolour rows still fault, so `vdpmode` may still be raised), and the order of the prune and the owner's run is chosen there so the diagnose counts come from one line shape. `ci.yml` is edited once, in the last push of the phase.

## Progress

| Milestone | Phases | Plans | Status | Shipped |
|-----------|--------|-------|--------|---------|
| v1.0 | 4/4 | 62/62 | Complete | 2026-11-09 |
| v2.0 | 8/8 | 94/94 | Complete | 2026-11-12 |
| v2.1 | 3/3 | 15/15 | Complete | 2026-11-13 |
| v2.2 | 0/2 | 0/TBD | In progress | - |

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 16. Graphics I and II draw | 9/9 | Complete    | 2026-11-14 |
| 17. Re-run, merge and clean state | 0/4 | Planned | - |
