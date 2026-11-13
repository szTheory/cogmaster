# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.0 — SMS and Game Gear games play with sound in RetroArch

**Shipped:** 2026-11-09 (release v0.4.0; phase 4 PR #11, release PR #12, audit PR #13)
**Phases:** 4 | **Plans:** 62 | **Sessions:** 37 Runtime Log rows in STATE.md, this close included

### What Was Built
- Phase 1: the whole delivery path before any emulation: library, `cogmaster-run` and libretro core draw a hashed test card, built and released on six platforms plus big-endian and wasm32 lanes, installed into RetroArch by one README line (v0.1.0).
- Phase 2: an own Z80 that matches the public SingleStepTests vectors on every cycle, the tick-lattice bus, the Sega, no-mapper and Codemasters boards, ports, the Pause NMI and the first-fault trace (v0.2.0).
- Phase 3: VDP mode 4 on both machines, the Game Gear window, two pads, regions, battery saves and the compatibility ledger, with owned fixtures playing in a hosted RetroArch (v0.3.0).
- Phase 4: the SN76489 with Game Gear stereo and the YM2413 for the Japanese profile, an exact 801 or 802 samples per frame, `--hash-audio`, and a hosted RetroArch recording against a silent control (v0.4.0).
- 26 of 26 requirements, 399 of 399 tests in the `ci` workflow, frame and audio hashes byte-identical on the eight hosted lanes.

### What Worked
- One pull request, one automated release per phase: four releases (v0.1.0 to v0.4.0) over the 22 days, each merged on green CI, with the audit finding 18 of 18 connections wired and 6 of 6 flows complete.
- Proving the CPU against public vectors before any game ran: the vector suite ran 160,400 executed cases before the VDP existed.
- Owned fixtures with a RAM mailbox and frozen pins in text files, compared on every lane through `hashes.txt`: determinism was shown by equal bytes, not asserted.
- A lockstep oracle for a port: the YM2413 matched the unmodified MIT emu2413 over 2,000,000 seeded samples before it touched the bus, and the run found a real 16-bit overflow in the chip's sum.
- Automation-first verification: the owner verified nothing by hand, and the hosted RetroArch checks stayed inside the 300-line budget.

### What Was Inefficient
- Provisional CI timeouts were set before the code existed. The first hosted run of the phase 4 head failed GCC, MSVC, UBSan and macOS x64 on timeouts, and the windows-arm64 timeout (3 minutes) later failed a main run.
- Review dispositions were not triaged in their phase: phase 1 still has 12 open rows, two of them live warnings (WR-02, WR-04), and phases 2 and 4 carry open info findings.
- Budgets were left at their ceiling: `tests/retroarch` ends at 299 of 300 lines and `docs/testing.md` at 7 bytes of headroom, so the next change must pay for itself.
- Work that was meant to follow a phase did not: the perf gate is still report-only with a `boot_to_play` allow line to 2027-01-07, and nightly lanes have not run on the sound code.
- The README did not move with the scope: its Quick start still covers only the no-content test card.
- The execution chain was one sequential line per phase (62 plans, parallelization off); Phase 4 alone had 15 plans in 15 waves.

### Patterns Established
- Each phase ends in a release: Conventional Commit pull request title, `gh pr merge --auto --squash`, CI run confirmed on the head commit, release pull request confirmed green.
- Evidence is a command, an exit status and a run URL, never a committed log.
- Closed-form unit oracles for chips, owned WLA-DX fixtures for behaviour, frozen hash pins for determinism, and an independent decoder or recording as the control for hosted checks.
- Every repository check ships with a negative control that fails with its named message.

### Key Lessons
1. Measure a CI timeout on the first hosted run of the code it covers before leaving it labelled provisional; a guess from an earlier phase does not hold when the per-frame work grows.
2. Triage review dispositions at the end of the phase that produced them; at milestone close they are debt that has lost its context.
3. When a budget is within a line or a few bytes of its cap, raise or shrink it in the same plan that reaches it, not in the plan that needs one more line.
4. Move the README with the scope in the phase that changes what a user can do, not after the audit.
5. Port third-party logic against a frozen digest of the unmodified original before wiring it to the bus; the oracle found a defect the register tests could not.

### Cost Observations
- Model mix by Runtime Log row: 21 claude-sonnet-5-5 (57%), 8 GPT-6 under Codex (22%), 7 claude-opus-5-5 (19%), 1 claude-fable-5-1 (3%); phase 4 planning used claude-opus-5-5 for the planner.
- Sessions: 37 Runtime Log rows, one GSD step per session.
- Notable: phases 1 and 2 were planned and executed on Opus, phase 3 on Codex and phase 4 on Sonnet; all four passed verification.

### Tech Debt Carried Forward
From `.planning/milestones/v1.0-MILESTONE-AUDIT.md` (status tech_debt, no gaps):
- Phase 1: provisional CI timeouts; untriaged review dispositions (WR-02, WR-04 live); the post-merge CI follow-ups todo (perf gate in report mode, nightly issue path unexercised, no dependabot configuration, wasm32 report-only).
- Phase 2: review findings IN-01 to IN-06 deferred; fuzz targets do not drain audio or the trace log.
- Phase 3: README Quick start covers the test card only.
- Phase 4: `boot_to_play` perf allow line to 2027-01-07; hosted sound check proves non-silence rather than PCM equality; review IN-01 to IN-04 open; be32 and wasm32 audio timings provisional; nightly lanes not yet run on the sound code; `cogmaster_info.audio_channels` and `.fm` read only by tests; third-party SN76489 and YM2413 ROM conformance deferred with no lane.

## Milestone: v2.0 — The whole library

**Shipped:** 2026-11-12 (release v0.12.0; phase 12 PR #33, release PR #34, audit PR #35)
**Phases:** 8 | **Plans:** 94 | **Sessions:** 84 Runtime Log rows in STATE.md since the v1.0 close, this close included

### What Was Built
- Phase 5: the v1.0 debt merged away: timeouts derived from live runs, the perf gate armed, `quality.json` floors and ceilings, Dependabot, the weekly digest and the README's ROM step (v0.5.0).
- Phase 6: the Korean, Zemina, Nemesis, 4 PAK and Hi-Com boards detected hash-first from binary SHA-1 rows, with a bounded scan below a valid header and exit 77 plus a reason for boards that stay unsupported (v0.6.0).
- Phase 7: the 93C46 EEPROM and battery saves that survive a restart in the runner and in a hosted RetroArch, with `.srm` interop only in measured layouts (v0.7.0).
- Phase 8: one pin-level port layer with the Sports Pad, Paddle and 3-D glasses behind a grown, backward-compatible input struct (v0.8.0).
- Phase 9: the Light Phaser, sensed from the emitted pixel at the beam position (v0.9.0).
- Phase 10: BIOS boot from the user's own dump with two firmware legality scans (v0.10.0).
- Phase 11: PAL timing proven on every lane and PAL and Japanese FM rows through the library judge (v0.11.0).
- Phase 12: one honest ledger row per Master System and Game Gear entry at the pin, a bounded compatibility page and `compat.exit` tying `is_experimental` to the judged rows; the flag stays until the owner's own-library run (v0.12.0).
- 49 of 49 requirements, 671 of 671 tests in the `ci` workflow (399 at the v1.0 close); API 9 to 14, state format 11 to 16, behaviour revision 17, and every phase kept the frame and audio pins that existed before it.

### What Worked
- Fan-out discuss with an adversarial pass: the Runtime Log shows five to six advisors plus an adversarial cross-check per discuss step, and the decisions that came out held; the milestone audit found 0 blockers.
- A draft pull request opened early to exercise a CI change before it was on main: the perf gate failed a +4.87 percent change on throwaway draft pull request 18 before it was trusted, and the ledger bridge in `ci.yml` ran on draft pull request #31.
- Code review that found real defects: 118 findings across the eight phases, 2 critical (both fixed), 56 fixed, 62 seeded or skipped with a reason, one fix round per phase against a limit of two.
- One release per phase held: eight phase pull requests and eight release pull requests, each published with 19 assets, each merged on green CI.
- Writing an owner decision into a test instead of a note: the 2026-11-12 decision that `is_experimental` stays until the owner's own run became `compat.exit`, which fails both ways (flag removed with floors unmet, floors met with the flag present).
- Hand-offs kept out of the gates: every VERIFICATION says `human_needed: false`, and the owner's runs are batched into SEED-013, SEED-010 and SEED-009 as `not_run` items.

### What Was Inefficient
- Provisional timeouts needed rework again: the phase 9 gun test went from a provisional 120 s to 20 s, phases 10 and 12 each spent a plan settling timeouts from hosted runs, and at the audit asan, the nightly bench and bench-wasm32 had drifted below twice their slowest run (W1). `hygiene.timeout_comments` reads only the comment format, so CI could not see it.
- Hosted runner DNS flakiness: on the first hosted dispatch of phase 10 the perf, be32 and wasm32 jobs failed at their `apt-get` step three times and on main, which left those legs unproven until a later dispatch.
- The rehearsal cleanup race: the release rehearsal read a deleted tag back once and failed once (SEED-014); it was fixed in a review round, but the rehearsal still adds about six minutes to any pull request that edits `ci.yml` or a release script.
- A flag-removal checklist that was incomplete: the SEED-013 list missed `docs/testing.md` and the page's second Experimental sentence until the audit (W4), and its flag-absent control tree omits both, so that proof is vacuous for them.
- Requirement wording was amended in fourteen requirements to match what could be proven in CI; each amendment landed with its code, but the as-written text lives in per-phase context files, not in one place.

### Patterns Established
- A throwaway draft pull request stacked on the phase branch proves a CI gate before it is on main.
- Every new test ships with a negative-control row, and every user doc claim is scoped by a check (`policy.claims_scoped`, `policy.no_title_table`).
- Owner-bound runs are `not_run` seeds with `human_judgment: true`, batched into one ask, never a requirement and never a gate.
- A timeout is `max(floor, 2 x the slowest hosted second)` from raw seconds over several runs, with the run named beside it.
- One function that returns a status, a message and lines (the exit command, the page) lets a self-test run its controls in process.

### Key Lessons
1. A check that reads only the format of a timeout comment cannot see the number being wrong; verify the arithmetic where it is stated, or re-measure at the end of every phase that adds runtime. This is the second milestone in which timeouts needed rework.
2. Triaging review findings inside the phase that produced them worked: 118 findings, none open and untriaged at the audit, against 12 open rows in phase 1 alone at the v1.0 close.
3. When an owner decision changes what a requirement can claim, put the decision in a command that fails both ways and amend the requirement text in the same plan.
4. A hand-off list is a requirement of its own: walk it against the code in the phase that writes it, because the audit found two lists that named the wrong files or commands (W4, W5).
5. Plan for hosted-runner outages as a normal event: a dispatch that fails at `apt-get` says nothing about the code, so record the leg as unproven and re-run rather than reading it as a failure.

### Cost Observations
- Model mix by Runtime Log row since the v1.0 close: 82 claude-sonnet-5-5 and 2 claude-sonnet-5-5 with a claude-opus-5-5 planner; every row ran under Claude Code.
- Sessions: 84 Runtime Log rows. Phases ran as autonomous sessions on the owner's authorisation, one GSD step per row.
- Notable: token and dollar cost were not measured; no number is claimed.

### Tech Debt Carried Forward
From `.planning/milestones/v2.0-MILESTONE-AUDIT.md` (status tech_debt, no gaps):
- Timeout drift (W1) and the leftover `Timeout: provisional` comments in `tests/fixtures/tests.cmake`; the 188-in-1 status word (W2); 50 Game Gear rows judged as `video=gg` (W3); the flag-removal control tree (W4); BIOS wording (W5); doc and CI claim drift (W6); libretro `pal` with Game Gear content (W7); the default-fm literal in five places (W8).
- Seeds: SEED-007 (phase 5 tooling), SEED-009 (scan thresholds), SEED-010 (pad and phaser leftovers, three judges), SEED-011 (BIOS), SEED-012 (hash-less audio predicate), SEED-013 (own-library run), SEED-014 (PAL and Japanese FM), SEED-015 (whole-list and CI speed).

---

## Milestone: v2.1 — Fewer failing cartridges

**Shipped:** 2026-11-13 (releases v0.14.0 to v0.14.2; phase PRs #40, #42 and #44, release PRs #41, #43 and #45, audit on the milestone branch)
**Phases:** 3 | **Plans:** 15 | **Sessions:** 22 Runtime Log rows in STATE.md since the v2.1 start, this close included

### What Was Built
- Phase 13: a counts-only boot diagnostic (`cogmaster-run --diagnose`, `sh scripts/library.sh diagnose`) with a canary leak scan and five negative controls, run over the owner's library, grouping the 205 failing default-profile rows by cause (v0.14.0).
- Phase 14: the causes fixed or closed with a stated reason, in group-size order: `$FFFC` unused-bit writes, writes to non-register ROM addresses and undriven or undecoded port accesses stopped being recorded as faults; the legacy video-mode rows were closed as not emulated by design; the boots check was shown sound; behaviour revision 18 and no hash pin moved (v0.14.1).
- Phase 15: the owner's library re-run merged through the unchanged ratchet (285 rows from `fail` to `pass`, none lost) and seven drifted CI timeouts re-derived from a fresh 40-run read in one workflow commit (v0.14.2).
- 10 of 10 requirements, 681 of 681 tests in the `ci` workflow (671 at the v2.0 close); API 14 and state format 16 unchanged, behaviour revision 17 to 18.

### What Worked
- Measuring before fixing: the diagnostic grouped 205 rows into a handful of leaves, and one cause (`$FFFC` writes) explained 109 of them, so fixing in group-size order took the diagnose from 205 to 104 to 69 to 49 failing rows with no leaf growing.
- A sourced decision table written before any core edit: each raise site had a decision and a document section, and count gates (D-04, D-07, D-08) meant plans 14-04 and 14-07 changed no code because the evidence did not call for it.
- The leak test as the permission to look: with a canary scan and negative controls over every output channel, the agent ran the diagnostic on dumps it may not read and committed counts, kinds and catalog titles only.
- The ratchet as the safety net for an unproven shift: the Sega bank-shift trace was not taken, and the Phase 15 run merged with no pass row lost, which is the evidence the trace would have been for.
- No target number for flipped rows: the causes decided the count, and a stricter check was never needed.

### What Was Inefficient
- SUMMARY front matter drifted from the rule: `requirements-completed` is empty in 13-01, 13-02 and 14-01 to 14-07, so the three-source matrix read FIX-01 and FIX-03 as partial though both were satisfied.
- Reports were not refreshed when their open item closed: `14-VERIFICATION.md` still reads `human_needed` for a pad press that `14-UAT.md` records as passed, and the Phase 13 verification went stale when Phase 14 changed five files it covers on purpose.
- A fix that removes behaviour did not sweep the prose for it: `docs/audio.md`, `docs/embedding.md` and `docs/runner.md` still describe port faults the core no longer records, and the port classes in `runner/diag_class.h` are dead vocabulary.
- The Phase 14 code-review step was skipped (denied by the auto-mode classifier) and is recorded only as advisory in its VERIFICATION.
- The ledger stamp names the pre-squash commit of PR #44, which is not an ancestor of `main`; the recorded ancestry check held only before the squash.

### Patterns Established
- Counts-only diagnostics: a closed-vocabulary record per item, a reducer that validates every record against fixed patterns, and a leak scan with one negative control per channel.
- A fix phase starts from a decision table (event, decision, document section) and gates every optional change on a measured count.
- Ledger rows carry the core commit that produced them as one `core=` stamp, and a library run is judged by the ratchet, never by hand.
- A timeout is re-derived from a fresh run read in one workflow commit per file, with `hygiene.timeout_comments` as the check.

### Key Lessons
1. When a core change removes a behaviour, grep the docs and the test vocabulary for its name in the same plan; the audit found three docs and a dead class list that the tests could not see.
2. Write `requirements-completed` in the plan that completes the requirement; the audit matrix treats an empty field as partial and a manual read has to override it.
3. Refresh the report when its open item closes: a UAT that passes should rewrite the VERIFICATION body in the same step, not leave `human_needed` for the audit to reconcile.
4. A stamp written before a squash merge names a commit that is not on `main`; record the merge-base or keep the branch until the stamp is replaced.
5. A third of the failing carts were one cause, not 200: group before fixing, and let the largest group set the order.

### Cost Observations
- Model mix by Runtime Log row since the v2.0 close: 17 claude-sonnet-5-5, 2 with sonnet advisors, 2 with a claude-opus-5-5 planner and a claude-haiku-5-5 plan checker (one also with a sonnet researcher), 1 with sonnet reviewer, fixer and verifier; every row ran under Claude Code.
- Sessions: 22 Runtime Log rows, one GSD step per row.
- Notable: token and dollar cost were not measured; no number is claimed.

### Tech Debt Carried Forward
From `.planning/milestones/v2.1-MILESTONE-AUDIT.md` (status tech_debt, no gaps):
- Stale docs and dead vocabulary after the Phase 14 fault removals; empty `requirements-completed` in the Phase 13 and 14 SUMMARYs; the `14-VERIFICATION.md` body; the pre-squash `core=` stamp; the dirty-tree check on the run path of `scripts/library.sh`; advisory WR-02 (`ram=live|still`); the open Phase 15 review items; the reg-0 board question (needs new evidence).
- Seeds: SEED-016 is done; SEED-006 now has the evidence its trigger names (46 failing rows need the legacy modes); SEED-007, SEED-009 to SEED-012, SEED-014 and SEED-015 stay dormant.

---

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Sessions | Phases | Key Change |
|-----------|----------|--------|------------|
| v1.0 | 37 | 4 | One phase, one pull request and one release each; automation-first verification |
| v2.0 | 84 | 8 | Fan-out discuss with an adversarial pass; draft pull request early for CI changes; owner hand-offs batched as seeds and kept out of the gates |
| v2.1 | 22 | 3 | Measure first: a counts-only diagnostic grouped the failures, a sourced decision table and count gates decided each fix, and the ratchet judged the re-run |

### Cumulative Quality

| Milestone | Tests (ci workflow) | Source lines (src and include) | Hashed lanes |
|-----------|---------------------|--------------------------------|--------------|
| v1.0 | 399 | about 7,700 | 8 hosted lanes, byte-identical |
| v2.0 | 671 | 9,403 in `src` | 8 hosted lanes, byte-identical; no earlier pin moved |
| v2.1 | 681 | 9,386 in `src` | no hash pin moved (behaviour revision 18) |

### Top Lessons (Verified Across Milestones)

1. Timeouts set before the code runs need a measurement afterwards, and the check must see the number, not only its comment (v1.0 lesson 1, v2.0 lesson 1 and the v2.1 phase 15 re-derivation of seven drifted timeouts: seen three times).
2. Triage review findings inside the phase that produced them (v1.0 lesson 2; v2.0 held it with one fix round per phase and no open untriaged finding; v2.1 skipped the Phase 14 code review, recorded as advisory).
3. Not yet repeated across milestones: budgets raised in the plan that reaches them, the README moving with the scope, and a frozen digest of the original before a port.
