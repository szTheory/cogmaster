# Requirements: cogmaster v2.0 The whole library

**Defined:** 2026-11-10
**Core Value:** Master System and Game Gear games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.

## v2.0 Requirements

Requirements for this milestone. Each maps to one roadmap phase. Scope decisions made with the owner on 2026-11-10: Mark III stays out; the ledger denominator is SMS and Game Gear only; the `is_experimental` exit is a command over the rows the owner's local library judged; Janggun and 4PAK are built if cheap, Seo Jin and 188-in-1 are named `unsupported`.

### Tune-up

- [x] **TUNE-01**: Every CI job timeout is derived from the slowest of the last 40 successful runs (or all of them if fewer, with N stated) with that run's ID beside it, and no timeout reads `provisional`
- [x] **TUNE-02**: A pull request that raises instructions per frame fails the perf gate, which is armed after two zero-spread control runs, and the `boot_to_play` allow line is deleted or restated with a current reason
- [x] **TUNE-03**: A weekly digest reports CI duration, flaky tests and open nightly issues without anyone asking, and a CI-efficiency report names the slowest job
- [x] **TUNE-04**: `quality.json` holds measured ceilings (including zip and wasm size) and floors that a CMake check enforces, and a `policy.lessons_unique` check runs in `ci`
- [x] **TUNE-05**: Behaviour prose lives in topic files so no `docs/*.md` file is within 2 KiB of its cap, and `tests/retroarch` has at least 20 lines of headroom
- [x] **TUNE-06**: The README says how to load a ROM and states in one sentence that no BIOS or game is shipped
- [x] **TUNE-07**: Each open review finding from phases 1, 2 and 4 (WR-02, WR-04, IN-01 to IN-06 and the phase 4 info items) is fixed, closed with a reason, or seeded
- [x] **TUNE-08**: The nightly lanes have run on the sound code, and the nightly report opens and closes an issue once on a forced failure
- [x] **TUNE-09**: Dependabot updates GitHub Actions weekly with minor and patch grouped, and a `policy.dependabot` test with a negative control guards its configuration
- [x] **TUNE-10**: The wasm32 lane is required by `CI required` once ten successful main runs show hashes equal to Linux x64's

### Mappers

- [x] **MAP-01**: The cartridge generator maps every pinned MAME slot to a board or to an explicit unsupported-by-design list, fails on an unmapped slot, and prints a count per slot
- [x] **MAP-02**: The hash table row holds a binary SHA-1 and fixed-width facts with titles moved to the ledger side, and `cartdb.c.o` is under 100 KB
- [x] **MAP-03**: A Korean `$A000`, Hicom, Zemina, Nemesis or `korean_nb` cartridge in the CC0 lists loads and plays with no setting
- [x] **MAP-04**: A Janggun or 4PAK cartridge plays, or its catalog row is `unsupported` and names why
- [x] **MAP-05**: Seo Jin and 188-in-1 appear as `unsupported` catalog rows that name what fails, and the runner exits 77 for them
- [x] **MAP-06**: A valid `TMR SEGA` header outranks the board scan, the scan needs a threshold and a margin, a tie refuses naming the candidate, and `--probe` reports `board_source`
- [x] **MAP-07**: Every new board has an exhaustive register-write and address-read test under ASan, and a documented register write records no fault
- [x] **MAP-08**: No existing frame or audio hash changes, shown by the eight-lane equality job

### Saves

- [x] **SAVE-01**: A Game Gear 93C46 game keeps its progress: the 128-byte image rides the battery span, the write-disabled power-up and EWEN, EWDS, WRITE, ERASE, WRAL and ERAL commands behave as the datasheet says, and the generation counter moves only on a completed change
- [x] **SAVE-02**: A mid-command EEPROM state round-trips through the save state, and the EEPROM bit-banger is a fuzz target
- [x] **SAVE-03**: The `.srm` sizes and EEPROM word order used by released libretro cores are measured once with the command and exit status recorded; import refuses lengths over the span, EEPROM import accepts only 0 or 128 bytes, and the layouts of other cores are claimed only where measured
- [x] **SAVE-04**: The libretro core reports saves enabled, with a guard test that the exported span equals the battery span
- [x] **SAVE-05**: The runner flushes saves every N completed frames with `--save-interval`, sweeps stale temporary files, and syncs the directory after the rename on POSIX
- [x] **SAVE-06**: A hosted RetroArch run writes a battery save and an EEPROM save, restarts, and reads them back, inside the 300-line budget

### Pad devices and 3-D glasses

- [x] **PAD-01**: A pin-level port layer routes every TH change through one edge detector, and the pad-only frame hashes and existing `io` tests do not change
- [x] **PAD-02**: A Sports Pad title tracks relative motion through the runner and libretro with tick-integer timeouts
- [x] **PAD-03**: A Paddle title works on Japan profiles, with deltas accumulated in state so a replay matches
- [x] **PAD-04**: The 3-D glasses eye written to `$FFF8-$FFFB` is reported with each frame's run result on every Master System profile and is 0 on the Game Gear, and the host composes the eyes outside the hashed frame
- [x] **PAD-05**: `cogmaster_set_input` accepts a grown struct by copying the smaller of the caller's size and its own (whole fields only) and zeroing the rest, so a caller built against the 32-byte `cogmaster_input` of API versions 1 to 11 is never read past its end
- [x] **PAD-06**: The hash table sets a default controller device per dump, the user option wins, profile availability is enforced, and the ledger key carries the device
- [x] **PAD-07**: Device state is bounded and cross-checked in the state visitor, and each device is in the fuzz target

### Light Phaser

- [x] **GUN-01**: The Light Phaser senses the emitted pixel at the beam position and latches H at the edge tick, with no per-title offset table
- [x] **GUN-02**: A run with the Light Phaser plugged in, the trigger never pulled and no light in the sensor's view has the same frame and audio hashes and the same machine state, apart from the gun's own fields, as a run with a pad and nothing pressed, and sensor work is bounded per picture and per line however often a game polls
- [x] **GUN-03**: A pure host helper maps a pointer through the rectangle of the last frame (the crop) to a field-dot aim, tested with Master System and Game Gear rectangles; libretro aims through it, the runner's script aim is the field dot itself (a Game Gear holds no Light Phaser), and the crosshair is drawn only into the libretro output buffer, outside the hashed frame
- [x] **GUN-04**: A self-authored fixture lights a known square and polls TH at several loop lengths, and the latched X matches within a stated tolerance, with the existing latch positions unchanged

### BIOS boot

- [x] **BIOS-01**: The two firmware legality scans (release and local window) exist before any BIOS code lands, each with a negative control; the release scan runs in `ci` and its controls with it, the local window scan runs in the `library` preset and the pre-commit and pre-push hooks, and its controls run in the `hygiene` lane
- [x] **BIOS-02**: A user boots a Master System from their own 8 KiB BIOS dump with `--boot bios --bios FILE` or the libretro option; fast boot stays the default, the dump is named by SHA-1, and the runner exits 77 when a BIOS is required and absent or not supported (the Game Gear and built-in-game BIOSes are seeds)
- [x] **BIOS-03**: CI runs a self-authored BIOS-shaped image with no skip, and a check shows the fast-boot hand-off equals the state that image reaches
- [x] **BIOS-04**: The libretro `.info` declares the firmware as optional and the adapter reads it from the system directory (three Master System file names; the Game Gear BIOS is a seed)
- [x] **BIOS-05**: BIOS and cartridge arbitration under `$3E` follows the documented hardware after SMS-SYSTEM section 3 is checked against the code, not an assumed exclusive overlay

### PAL and Japanese FM

- [x] **REG-01**: The ledger splits `tv` from `region` with a migration and a format note
- [x] **REG-02**: PAL fixtures (V-counter tables, Pause at line 311, exact samples per frame) give equal hashes on all eight lanes
- [x] **REG-03**: An `audio@F=H` predicate over the transition stream, with a negative control, and `fm` in the row identity
- [x] **REG-04**: The library judge adds PAL and Japanese FM rows for catalog-identified carts through `compat-update.sh`, proven in CI by a synthetic library run through `library.sh run`

### Whole-list ledger

- [x] **LEDG-01**: The ledger holds one default-profile row for every SMS and Game Gear catalog identity at the pin (an image SHA-1, or for the 19 compound entries the SHA-1 of `mame/<list>/<name>`); the generator prints the excluded SG-1000 count and the reconciliation 1,476 records = 1,444 images + 19 compound + 13 same-image aliases, and a test equates the ledger's identities with the printed counts
- [x] **LEDG-02**: Every `unsupported` row carries a closed reason token and the slot table refuses its SHA-1 (the runner exits 77), proven by a unit loop over every board-0 row and runner runs of synthetic images where a signature exists; every MAME `supported="no"` entry is `skipped` with the reason `mame_no`; a row without a valid token fails the parser
- [x] **LEDG-03**: The ratchet allows an `unsupported` row to become `pass` once the board or device exists (its reason token is `retired` and the catalog no longer says `unsupported`), with failing negative controls; the pull request that adds the rule is judged by the previous rules
- [x] **LEDG-04**: `docs/compatibility.md` is aggregates plus the evaluated rows the budget allows (fixtures, up to 20 `unsupported` rows, up to 40 `fail` rows, up to 40 others, a `+K more` line each past its cap) and stays inside its byte budget
- [x] **LEDG-05**: A command over the ledger's judged rows (`compat.exit`) exists and is proven in CI on the public committed rows and the synthetic library's merged ledger; a test ties `is_experimental` to the committed ledger's judged-cart floors both ways (present while unmet, absent once met); every `unsupported` row names its reason; unowned rows stay `unknown` with the count printed, including the PAL and Japanese FM rows once the owner's run has added them

## v2.x Requirements

Deferred to a later release. Tracked but not in the roadmap.

### Devices and boot

- **FUT-01**: Mega Drive pad on the pin-level ports
- **FUT-02**: Built-in-game BIOSes (SMS2 128 KB and 256 KB)
- **FUT-03**: A `.srm` pad-on-import rule, only after the measured layouts show one is needed

## Out of Scope

| Feature | Reason |
|---------|--------|
| Mark III profile | Contested in research; the media value stays reserved until the owner asks |
| SG-1000 and SC-3000 mode and rows | SEED-006 holds it; the denominator is SMS and Game Gear only |
| The browser build | SEED-008 stays dormant |
| Per-title Light Phaser offset tables | A behaviour hack; one global delay constant, any per-title fact lives in the generated database with evidence |
| Guessing a foreign save layout | Interop claims only what was measured |
| A bundled or required BIOS | Copyright and ASSET_POLICY; the user's dump only |
| Gear-to-Gear cable, SK-1100, Graphic Board, cheat devices | Named `unsupported` with exit 77 |
| Crosshair or anaglyph in the hashed frame | Would make hashes depend on the host |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| TUNE-01 | Phase 5 | Complete |
| TUNE-02 | Phase 5 | Complete |
| TUNE-03 | Phase 5 | Complete |
| TUNE-04 | Phase 5 | Complete |
| TUNE-05 | Phase 5 | Complete |
| TUNE-06 | Phase 5 | Complete |
| TUNE-07 | Phase 5 | Complete |
| TUNE-08 | Phase 5 | Complete |
| TUNE-09 | Phase 5 | Complete |
| TUNE-10 | Phase 5 | Complete |
| MAP-01 | Phase 6 | Complete |
| MAP-02 | Phase 6 | Complete |
| MAP-03 | Phase 6 | Complete |
| MAP-04 | Phase 6 | Complete |
| MAP-05 | Phase 6 | Complete |
| MAP-06 | Phase 6 | Complete |
| MAP-07 | Phase 6 | Complete |
| MAP-08 | Phase 6 | Complete |
| SAVE-01 | Phase 7 | Complete |
| SAVE-02 | Phase 7 | Complete |
| SAVE-03 | Phase 7 | Complete |
| SAVE-04 | Phase 7 | Complete |
| SAVE-05 | Phase 7 | Complete |
| SAVE-06 | Phase 7 | Complete |
| PAD-01 | Phase 8 | Complete |
| PAD-02 | Phase 8 | Complete |
| PAD-03 | Phase 8 | Complete |
| PAD-04 | Phase 8 | Complete |
| PAD-05 | Phase 8 | Complete |
| PAD-06 | Phase 8 | Complete |
| PAD-07 | Phase 8 | Complete |
| GUN-01 | Phase 9 | Complete |
| GUN-02 | Phase 9 | Complete |
| GUN-03 | Phase 9 | Complete |
| GUN-04 | Phase 9 | Complete |
| BIOS-01 | Phase 10 | Complete |
| BIOS-02 | Phase 10 | Complete |
| BIOS-03 | Phase 10 | Complete |
| BIOS-04 | Phase 10 | Complete |
| BIOS-05 | Phase 10 | Complete |
| REG-01 | Phase 11 | Complete |
| REG-02 | Phase 11 | Complete |
| REG-03 | Phase 11 | Complete |
| REG-04 | Phase 11 | Complete |
| LEDG-01 | Phase 12 | Complete |
| LEDG-02 | Phase 12 | Complete |
| LEDG-03 | Phase 12 | Complete |
| LEDG-04 | Phase 12 | Complete |
| LEDG-05 | Phase 12 | Complete |

**Coverage:**
- v2.0 requirements: 49 total
- Mapped to phases: 49
- Unmapped: 0 ✓

---
*Requirements defined: 2026-11-10*
*Last updated: 2026-11-12 after the v2.0 milestone audit (49 of 49 complete; LEDG-05 as amended)*
