# Roadmap: cogmaster

## Milestones

- ✅ **v1.0 SMS and Game Gear games play with sound in RetroArch** — Phases 1-4 (shipped 2026-11-09, releases v0.1.0 to v0.4.0)
- 🚧 **v2.0 The whole library** — Phases 5-12 (in progress)

## Phases

<details>
<summary>✅ v1.0 SMS and Game Gear games play with sound in RetroArch (Phases 1-4) — SHIPPED 2026-11-09</summary>

- [x] Phase 1: A test frame in RetroArch (12/12 plans) — completed 2026-10-19
- [x] Phase 2: The CPU matches the public vectors (17/17 plans) — completed 2026-10-20
- [x] Phase 3: Games play in RetroArch (18/18 plans) — completed 2026-10-20
- [x] Phase 4: Sound (15/15 plans) — completed 2026-11-09

Full phase details: `.planning/milestones/v1.0-ROADMAP.md`. Requirements: `.planning/milestones/v1.0-REQUIREMENTS.md`. Audit: `.planning/milestones/v1.0-MILESTONE-AUDIT.md`.

</details>

### 🚧 v2.0 The whole library (Phases 5-12)

**Milestone goal:** The cartridges and peripherals a collector owns all play, with saves and the PAL and Japan
profiles, checked against the whole CC0 MAME ledger.

The tune-up comes first so the perf gate, timeouts and budgets are measured against current `main` before the bus
and I/O paths gain branches. The Korean mappers follow, because they change the hash row layout every later phase
reads. Saves ride the mapper split (the 93C46 board is a Sega variant). The pin-level port layer and the grown input
struct ship with the Sports Pad, Paddle and 3-D glasses, and the Light Phaser ships alone afterwards, because it is the
only item that depends on VDP latch timing. BIOS boot lands after the bus paths are proven, with its legality guards
first. PAL and Japanese FM rows need the ledger's `tv` split and an audio predicate; the whole-list ledger and the
`is_experimental` exit come last, since they are the phase most able to overclaim. Every phase is one branch, one
pull request and one automated release.

- [ ] **Phase 5: Tune-up** - The v1.0 debt merged away: measured timeouts, an armed perf gate, `quality.json`, budget headroom, the README's ROM step and the weekly digest
- [ ] **Phase 6: Korean cartridges play** - The remaining mapper boards load and play with no setting, detected hash-first, with no shipped hash moving
- [ ] **Phase 7: Saves survive a restart** - 93C46 EEPROM and battery saves persist through the runner and RetroArch, with `.srm` interop only in measured layouts
- [ ] **Phase 8: Sports Pad, Paddle and 3-D glasses** - Pin-level ports and a grown input struct make the pad-device and 3-D titles play, with replays matching
- [ ] **Phase 9: Light Phaser** - Light Phaser titles hit where the player aims, sensed from the emitted pixel at the beam
- [ ] **Phase 10: BIOS boot** - A user boots from their own BIOS dump in the runner or RetroArch; fast boot stays the default
- [ ] **Phase 11: PAL and Japanese FM** - PAL timing proven on every lane and the owner's PAL and FM carts judged under the right profile
- [ ] **Phase 12: The whole list in the ledger** - Every SMS and Game Gear entry at the pin has an honest row, and `is_experimental` leaves the core

## Phase Details

### Phase 5: Tune-up
**Goal**: The v1.0 debt is merged: CI timeouts, the perf gate and size ceilings are measured and armed against current `main`, the docs and hosted-test budgets have room for this milestone, CI reports on itself weekly, and the README tells a player how to load a ROM.
**Depends on**: Phase 4 (v1.0 shipped)
**Requirements**: TUNE-01, TUNE-02, TUNE-03, TUNE-04, TUNE-05, TUNE-06, TUNE-07, TUNE-08, TUNE-09, TUNE-10
**Success Criteria** (what must be TRUE):
  1. Every job timeout in the CI workflow carries the run ID of the slowest of that job's last 40 successful runs, or all of them if fewer, with N stated, and no timeout reads `provisional`; the wasm32 lane is required by `CI required` after ten successful main runs whose hashes equal Linux x64's (TUNE-01, TUNE-10).
  2. A control change that raises instructions per frame fails the armed perf gate on a pull request; the gate was armed after two zero-spread control runs, and the perf allow list has no `boot_to_play` line or one with a current reason (TUNE-02).
  3. The `quality.json` ceilings and floors are enforced where each can be measured, each with a negative control that fails: `cmake --workflow --preset ci` checks the test-count floors and the docs cap, each `build` leg checks its zip sizes after packaging, and the wasm32 lane checks the wasm size; the `ci` preset also runs `policy.lessons_unique` and `policy.dependabot`, each with a negative control; the Dependabot configuration updates GitHub Actions weekly with minor and patch grouped (TUNE-04, TUNE-09).
  4. A scheduled weekly digest reports CI duration, flaky tests and open nightly issues and names the slowest job with no one asking; the nightly lanes have run on the sound code, and a forced nightly failure opens one issue that the next passing run closes, each recorded as a run URL (TUNE-03, TUNE-08).
  5. The budget test shows every `docs/*.md` file more than 2 KiB under its cap and `tests/retroarch` at least 20 lines under 300; the README says how to load a ROM and states in one sentence that no BIOS or game is shipped; every open review finding from phases 1, 2 and 4 has a fixed, closed-with-reason or seeded disposition (TUNE-05, TUNE-06, TUNE-07).
**Plans**: 1/14 plans executed

Plans:
- [x] 05-01-PLAN.md — perf control runs on main first; docs split into topic files (TUNE-02, TUNE-05)
- [ ] 05-02-PLAN.md — hosted RetroArch test lines reclaimed, BMP reader folded into the comparator (TUNE-05)
- [ ] 05-03-PLAN.md — ci-durations report counting runs, tightened timeout check, measured timeouts, be32 decision (TUNE-01, TUNE-03)
- [ ] 05-04-PLAN.md — wasm32 promoted into hash-equality and CI required (TUNE-10)
- [ ] 05-05-PLAN.md — quality.json tracer: floors and docs cap in the ci preset, policy.lessons_unique (TUNE-04)
- [ ] 05-06-PLAN.md — quality.json expansion: base ratchet, zip and wasm size steps, first phase-branch run (TUNE-04)
- [ ] 05-07-PLAN.md — cartridge body over 1 MiB refused, with its test and doc wording (TUNE-07)
- [ ] 05-08-PLAN.md — hardware and tooling comment corrections, moved-docs pointers (TUNE-07)
- [ ] 05-09-PLAN.md — review dispositions settled in place; tooling findings and sound ROM deferral seeded (TUNE-07)
- [ ] 05-10-PLAN.md — README "Play a game" with the no-BIOS sentence and its check (TUNE-06)
- [ ] 05-11-PLAN.md — rehearsal skip with the CI required author clause and its control (TUNE-09)
- [ ] 05-12-PLAN.md — Dependabot with the minor-and-patch group, auto-merge, policy.dependabot (TUNE-09)
- [ ] 05-13-PLAN.md — nightly issue open-and-close on main with the sound-merge ancestry check; weekly digest (TUNE-08, TUNE-03)
- [ ] 05-14-PLAN.md — perf gate armed last, draft-PR proof, final-head green run, ship-time list (TUNE-02)

### Phase 6: Korean cartridges play
**Goal**: A collector's Korean-board cartridges in the CC0 lists load and play with no setting, detection never turns a Sega game into a Korean board, unsupported boards say why, and no shipped frame or audio hash moves.
**Depends on**: Phase 5
**Requirements**: MAP-01, MAP-02, MAP-03, MAP-04, MAP-05, MAP-06, MAP-07, MAP-08
**Success Criteria** (what must be TRUE):
  1. The cartridge generator prints a count per MAME slot, maps every pinned slot to a board or to the unsupported-by-design list, and fails when fed an unmapped slot; the hash table holds binary SHA-1 rows with titles on the ledger side, and a size check keeps `cartdb.c.o` under 100 KB (MAP-01, MAP-02).
  2. Korean `$A000`, Hicom, Zemina, Nemesis and `korean_nb` images play through the runner and the libretro core with no option set; a Janggun or 4PAK image plays or its ledger row is `unsupported` with a reason; Seo Jin and 188-in-1 rows are `unsupported`, name what fails, and the runner exits 77 for them (MAP-03, MAP-04, MAP-05).
  3. `cogmaster-run --probe` reports `board_source`; a decoy with a valid `TMR SEGA` header and Korean-looking bytes stays a Sega board, a scan below threshold or margin does not pick a Korean board, and a tie refuses with the candidate named (MAP-06).
  4. Each new board passes an exhaustive register-write and address-read test under ASan, and a test shows its documented register writes record no fault (MAP-07).
  5. The eight-lane equality job shows every frame and audio hash that existed before the phase unchanged (MAP-08).
**Plans**: TBD

### Phase 7: Saves survive a restart
**Goal**: A Game Gear 93C46 game and a battery-backed game keep their progress across restarts in the runner and in RetroArch, saves survive a killed process, and `.srm` files cross between cogmaster and other released cores only in layouts that were measured.
**Depends on**: Phase 6 (the EEPROM board kind and persistence column come from the mapper split)
**Requirements**: SAVE-01, SAVE-02, SAVE-03, SAVE-04, SAVE-05, SAVE-06
**Success Criteria** (what must be TRUE):
  1. EEPROM tests show the write-disabled power-up and the EWEN, EWDS, WRITE, ERASE, WRAL and ERAL commands behaving as the datasheet says, the 128-byte image riding the battery span, and the generation counter moving only on a completed change; a mid-command state round-trips byte for byte, and the bit-banger runs in the fuzz target (SAVE-01, SAVE-02).
  2. The `.srm` sizes and EEPROM word order of released libretro cores are measured once, recorded as a command and exit status in the phase summary, and an import test accepts exactly the measured layouts and refuses others (SAVE-03).
  3. The libretro core reports saves enabled, and a guard test shows the exported span equals the battery span (SAVE-04).
  4. `cogmaster-run --save-interval N` writes the save after every N completed frames; a kill test between flushes leaves the last complete save readable, stale temporary files are swept on the next run, and the directory is synced after the rename on POSIX (SAVE-05).
  5. A hosted RetroArch run writes a battery save and an EEPROM save, restarts and reads both back, and the budget test keeps `tests/retroarch` at or under 300 lines (SAVE-06).
**Plans**: TBD

### Phase 8: Sports Pad, Paddle and 3-D glasses
**Goal**: Sports Pad, Paddle and 3-D glasses titles play through the runner and the libretro core with the right device chosen per dump, peripheral input is recorded so replays match, and every pad-only hash stays fixed.
**Depends on**: Phase 6 (the hash row carries the default device); runs after Phase 7
**Requirements**: PAD-01, PAD-02, PAD-03, PAD-04, PAD-05, PAD-06, PAD-07
**Success Criteria** (what must be TRUE):
  1. Every TH change goes through one edge detector in the pin-level port layer, and the pad-only frame hashes and the existing `io` tests are unchanged on all eight lanes (PAD-01).
  2. A Sports Pad fixture tracks relative motion through the runner and libretro with timeouts in integer ticks; a Paddle fixture works on a Japan profile and is refused on a profile without it, and replaying the recorded deltas gives the same frame hash (PAD-02, PAD-03).
  3. The 3-D glasses eye state is reported at frame end only while the cartridge enable bit is set, and the frame hash is the same whether or not the host composes the eyes (PAD-04).
  4. A caller built against the version 9 `cogmaster_input` passes its smaller struct and an ASan test shows `cogmaster_set_input` never reads past it, copying the smaller size and zeroing the rest (PAD-05).
  5. A dump's default device comes from the hash table with no option, a user option overrides it, the ledger key carries the device, the state visitor refuses out-of-range device state, and each device is in the fuzz target (PAD-06, PAD-07).
**Plans**: TBD

### Phase 9: Light Phaser
**Goal**: Light Phaser titles hit where the player aims, sensed from the emitted pixel at the beam position with H latched at the edge tick, without changing any run that has no gun or fires no shot.
**Depends on**: Phase 8 (the port layer and TH edge detector)
**Requirements**: GUN-01, GUN-02, GUN-03, GUN-04
**Success Criteria** (what must be TRUE):
  1. A self-authored fixture lights a known square and polls TH at several loop lengths, and the latched X matches within the stated tolerance at every length, while the existing latch-position tests stay unchanged (GUN-04).
  2. The sensor reads the emitted pixel at the beam position and latches H at the edge tick, and a check finds no per-title offset table in `src/` (GUN-01).
  3. A run with the gun plugged in and no shot has the same frame hash as one without it, and a poll-hammer test shows sensor work bounded per line however often the game polls (GUN-02).
  4. Runner and libretro tests map aim through the SMS and Game Gear crops to field coordinates, and the frame hash is the same with the crosshair drawn or not (GUN-03).
**Plans**: TBD

### Phase 10: BIOS boot
**Goal**: A user boots from their own BIOS dump in the runner or RetroArch, fast boot stays the default, BIOS and cartridge arbitration follows the documented hardware, and no BIOS byte ever enters the tree or a release.
**Depends on**: Phase 8 (the proven `$3E` and bus paths); independent of Phase 9, so it may run first if the Light Phaser is blocked
**Requirements**: BIOS-01, BIOS-02, BIOS-03, BIOS-04, BIOS-05
**Success Criteria** (what must be TRUE):
  1. The release firmware scan and the local firmware-window scan run in `ci`, each with a negative control that fails, and they land in a commit before any BIOS code (BIOS-01).
  2. `cogmaster-run --boot bios --bios FILE` boots the image, a known dump is recognised by SHA-1, fast boot stays the default with every existing hash unchanged, and the runner exits 77 when a BIOS is required and absent (BIOS-02).
  3. CI runs a self-authored BIOS-shaped image with no skip, and a check shows the fast-boot hand-off state equals the state that image reaches (BIOS-03).
  4. The libretro `.info` declares the firmware as optional, and an in-process host test shows the adapter reads it from the system directory when the boot option asks for it (BIOS-04).
  5. SMS-SYSTEM section 3 is checked against the code, and a test exercises each BIOS and cartridge enable combination under `$3E` with the documented result (BIOS-05).
**Plans**: TBD

### Phase 11: PAL and Japanese FM
**Goal**: PAL timing is proven identical on every lane, FM rows are judged by their sound, and the owner's PAL and Japanese FM cartridges are judged under the right profile and merged into the ledger.
**Depends on**: Phase 8 (the ledger key with the device); runs after Phase 10
**Requirements**: REG-01, REG-02, REG-03, REG-04
**Success Criteria** (what must be TRUE):
  1. A ledger migration splits `tv` from `region` with a format note, and every existing row keeps its status through it (REG-01).
  2. PAL fixtures for the V-counter tables, Pause at line 311 and exact samples per frame give equal hashes on all eight lanes (REG-02).
  3. The `audio@F=H` predicate passes on a matching transition-stream hash and fails on its negative control, and `fm` is part of the row identity (REG-03).
  4. A local library run judges the owner's PAL and Japanese FM carts, `compat-update.sh` merges the rows and the ratchet passes, recorded as a command and exit status (REG-04).
**Plans**: TBD

### Phase 12: The whole list in the ledger
**Goal**: Every SMS and Game Gear entry at the MAME pin has a ledger row with an honest status, and the libretro core drops `is_experimental` when the command over the owner's judged rows passes.
**Depends on**: Phases 6 to 11 (every board, device and profile built or named `unsupported`)
**Requirements**: LEDG-01, LEDG-02, LEDG-03, LEDG-04, LEDG-05
**Success Criteria** (what must be TRUE):
  1. The ledger holds a row for every SMS and Game Gear entry at the pin, a test equates the row count with the generator's, and the generator prints the excluded SG-1000 count (LEDG-01).
  2. Every `unsupported` row names its reason and the runner exits 77 for it, every MAME `supported="no"` entry is `skipped` with a reason, and a row without a reason fails the check (LEDG-02).
  3. The ratchet lets an `unsupported` row become `pass` once its board or device exists, and its negative control fails (LEDG-03).
  4. `docs/compatibility.md` holds aggregates plus evaluated rows and stays inside its byte budget (LEDG-04).
  5. The exit command over the owner's judged rows passes, the libretro `.info` no longer carries `is_experimental`, and the count of unowned `unknown` rows is printed (LEDG-05).
**Plans**: TBD

## Progress

| Milestone | Phases | Plans | Status | Shipped |
|-----------|--------|-------|--------|---------|
| v1.0 | 4/4 | 62/62 | Complete | 2026-11-09 |
| v2.0 | 0/8 | 0/TBD | In progress | - |

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 5. Tune-up | v2.0 | 1/14 | In Progress | - |
| 6. Korean cartridges play | v2.0 | 0/TBD | Not started | - |
| 7. Saves survive a restart | v2.0 | 0/TBD | Not started | - |
| 8. Sports Pad, Paddle and 3-D glasses | v2.0 | 0/TBD | Not started | - |
| 9. Light Phaser | v2.0 | 0/TBD | Not started | - |
| 10. BIOS boot | v2.0 | 0/TBD | Not started | - |
| 11. PAL and Japanese FM | v2.0 | 0/TBD | Not started | - |
| 12. The whole list in the ledger | v2.0 | 0/TBD | Not started | - |
