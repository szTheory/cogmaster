# Requirements: cogmaster v2.0 The whole library

**Defined:** 2026-11-10
**Core Value:** Master System and Game Gear games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.

## v2.0 Requirements

Requirements for this milestone. Each maps to one roadmap phase. Scope decisions made with the owner on 2026-11-10: Mark III stays out; the ledger denominator is SMS and Game Gear only; the `is_experimental` exit is a command over the rows the owner's local library judged; Janggun and 4PAK are built if cheap, Seo Jin and 188-in-1 are named `unsupported`.

### Tune-up

- [ ] **TUNE-01**: Every CI job timeout is derived from the slowest of the last 40 successful runs (or all of them if fewer, with N stated) with that run's ID beside it, and no timeout reads `provisional`
- [ ] **TUNE-02**: A pull request that raises instructions per frame fails the perf gate, which is armed after two zero-spread control runs, and the `boot_to_play` allow line is deleted or restated with a current reason
- [ ] **TUNE-03**: A weekly digest reports CI duration, flaky tests and open nightly issues without anyone asking, and a CI-efficiency report names the slowest job
- [ ] **TUNE-04**: `quality.json` holds measured ceilings (including zip and wasm size) and floors that a CMake check enforces, and a `policy.lessons_unique` check runs in `ci`
- [ ] **TUNE-05**: Behaviour prose lives in topic files so no `docs/*.md` file is within 2 KiB of its cap, and `tests/retroarch` has at least 20 lines of headroom
- [ ] **TUNE-06**: The README says how to load a ROM and states in one sentence that no BIOS or game is shipped
- [ ] **TUNE-07**: Each open review finding from phases 1, 2 and 4 (WR-02, WR-04, IN-01 to IN-06 and the phase 4 info items) is fixed, closed with a reason, or seeded
- [ ] **TUNE-08**: The nightly lanes have run on the sound code, and the nightly report opens and closes an issue once on a forced failure
- [ ] **TUNE-09**: Dependabot updates GitHub Actions weekly with minor and patch grouped, and a `policy.dependabot` test with a negative control guards its configuration
- [ ] **TUNE-10**: The wasm32 lane is required by `CI required` once ten successful main runs show hashes equal to Linux x64's

### Mappers

- [ ] **MAP-01**: The cartridge generator maps every pinned MAME slot to a board or to an explicit unsupported-by-design list, fails on an unmapped slot, and prints a count per slot
- [ ] **MAP-02**: The hash table row holds a binary SHA-1 and fixed-width facts with titles moved to the ledger side, and `cartdb.c.o` is under 100 KB
- [ ] **MAP-03**: A Korean `$A000`, Hicom, Zemina, Nemesis or `korean_nb` cartridge in the CC0 lists loads and plays with no setting
- [ ] **MAP-04**: A Janggun or 4PAK cartridge plays, or its ledger row is `unsupported` and names why
- [ ] **MAP-05**: Seo Jin and 188-in-1 appear as `unsupported` rows that name what fails, and the runner exits 77 for them
- [ ] **MAP-06**: A valid `TMR SEGA` header outranks the board scan, the scan needs a threshold and a margin, a tie refuses naming the candidate, and `--probe` reports `board_source`
- [ ] **MAP-07**: Every new board has an exhaustive register-write and address-read test under ASan, and a documented register write records no fault
- [ ] **MAP-08**: No existing frame or audio hash changes, shown by the eight-lane equality job

### Saves

- [ ] **SAVE-01**: A Game Gear 93C46 game keeps its progress: the 128-byte image rides the battery span, the write-disabled power-up and EWEN, EWDS, WRITE, ERASE, WRAL and ERAL commands behave as the datasheet says, and the generation counter moves only on a completed change
- [ ] **SAVE-02**: A mid-command EEPROM state round-trips through the save state, and the EEPROM bit-banger is a fuzz target
- [ ] **SAVE-03**: The `.srm` sizes and EEPROM word order used by released libretro cores are measured once with the command and exit status recorded, and import accepts only the measured layouts
- [ ] **SAVE-04**: The libretro core reports saves enabled, with a guard test that the exported span equals the battery span
- [ ] **SAVE-05**: The runner flushes saves every N completed frames with `--save-interval`, sweeps stale temporary files, and syncs the directory after the rename on POSIX
- [ ] **SAVE-06**: A hosted RetroArch run writes a battery save and an EEPROM save, restarts, and reads them back, inside the 300-line budget

### Pad devices and 3-D glasses

- [ ] **PAD-01**: A pin-level port layer routes every TH change through one edge detector, and the pad-only frame hashes and existing `io` tests do not change
- [ ] **PAD-02**: A Sports Pad title tracks relative motion through the runner and libretro with tick-integer timeouts
- [ ] **PAD-03**: A Paddle title works on Japan profiles, with deltas accumulated in state so a replay matches
- [ ] **PAD-04**: The 3-D glasses eye state is reported at frame end, gated by the cartridge enable bit, and the host composes the eyes outside the hashed frame
- [ ] **PAD-05**: `cogmaster_set_input` accepts a grown struct by copying the smaller of the caller's size and its own and zeroing the rest, so a version 9 caller is never read past its end
- [ ] **PAD-06**: The hash table sets a default controller device per dump, the user option wins, profile availability is enforced, and the ledger key carries the device
- [ ] **PAD-07**: Device state is bounded and cross-checked in the state visitor, and each device is in the fuzz target

### Light Phaser

- [ ] **GUN-01**: The Light Phaser senses the emitted pixel at the beam position and latches H at the edge tick, with no per-title offset table
- [ ] **GUN-02**: A run with the gun plugged in and no shot has the same frame hash as one without it, and sensor work is bounded per line however often a game polls
- [ ] **GUN-03**: The runner and libretro map aim through the crop to field coordinates, and the crosshair is drawn outside the hashed frame
- [ ] **GUN-04**: A self-authored fixture lights a known square and polls TH at several loop lengths, and the latched X matches within a stated tolerance, with the existing latch positions unchanged

### BIOS boot

- [ ] **BIOS-01**: The two firmware legality scans (release and local window) exist, each with a negative control, before any BIOS code lands
- [ ] **BIOS-02**: A user boots from their own BIOS dump with `--boot bios --bios FILE` or the libretro option, fast boot stays the default, the dump is recognised by SHA-1, and the runner exits 77 when a BIOS is required and absent
- [ ] **BIOS-03**: CI runs a self-authored BIOS-shaped image with no skip, and a check shows the fast-boot hand-off equals the state that image reaches
- [ ] **BIOS-04**: The libretro `.info` declares the firmware as optional and the adapter reads it from the system directory
- [ ] **BIOS-05**: BIOS and cartridge arbitration under `$3E` follows the documented hardware after SMS-SYSTEM section 3 is checked against the code, not an assumed exclusive overlay

### PAL and Japanese FM

- [ ] **REG-01**: The ledger splits `tv` from `region` with a migration and a format note
- [ ] **REG-02**: PAL fixtures (V-counter tables, Pause at line 311, exact samples per frame) give equal hashes on all eight lanes
- [ ] **REG-03**: An `audio@F=H` predicate over the transition stream, with a negative control, and `fm` in the row identity
- [ ] **REG-04**: PAL and Japanese FM rows are judged for the owner's carts through the local library path and merged by `compat-update.sh`

### Whole-list ledger

- [ ] **LEDG-01**: The ledger covers every SMS and Game Gear entry at the pin, and the generator prints the excluded SG-1000 count
- [ ] **LEDG-02**: Every `unsupported` row names its reason and exits 77, and a MAME `supported="no"` entry is `skipped` with a reason
- [ ] **LEDG-03**: The ratchet allows an `unsupported` row to become `pass` once the board or device exists, with a negative control
- [ ] **LEDG-04**: `docs/compatibility.md` is aggregates plus evaluated rows and stays inside its byte budget
- [ ] **LEDG-05**: `is_experimental` leaves the libretro `.info` when a command over the owner's judged rows passes, every `unsupported` row names its reason, and unowned rows stay `unknown` with the count printed

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
| TUNE-01 | Phase 5 | Pending |
| TUNE-02 | Phase 5 | Pending |
| TUNE-03 | Phase 5 | Pending |
| TUNE-04 | Phase 5 | Pending |
| TUNE-05 | Phase 5 | Pending |
| TUNE-06 | Phase 5 | Pending |
| TUNE-07 | Phase 5 | Pending |
| TUNE-08 | Phase 5 | Pending |
| TUNE-09 | Phase 5 | Pending |
| TUNE-10 | Phase 5 | Pending |
| MAP-01 | Phase 6 | Pending |
| MAP-02 | Phase 6 | Pending |
| MAP-03 | Phase 6 | Pending |
| MAP-04 | Phase 6 | Pending |
| MAP-05 | Phase 6 | Pending |
| MAP-06 | Phase 6 | Pending |
| MAP-07 | Phase 6 | Pending |
| MAP-08 | Phase 6 | Pending |
| SAVE-01 | Phase 7 | Pending |
| SAVE-02 | Phase 7 | Pending |
| SAVE-03 | Phase 7 | Pending |
| SAVE-04 | Phase 7 | Pending |
| SAVE-05 | Phase 7 | Pending |
| SAVE-06 | Phase 7 | Pending |
| PAD-01 | Phase 8 | Pending |
| PAD-02 | Phase 8 | Pending |
| PAD-03 | Phase 8 | Pending |
| PAD-04 | Phase 8 | Pending |
| PAD-05 | Phase 8 | Pending |
| PAD-06 | Phase 8 | Pending |
| PAD-07 | Phase 8 | Pending |
| GUN-01 | Phase 9 | Pending |
| GUN-02 | Phase 9 | Pending |
| GUN-03 | Phase 9 | Pending |
| GUN-04 | Phase 9 | Pending |
| BIOS-01 | Phase 10 | Pending |
| BIOS-02 | Phase 10 | Pending |
| BIOS-03 | Phase 10 | Pending |
| BIOS-04 | Phase 10 | Pending |
| BIOS-05 | Phase 10 | Pending |
| REG-01 | Phase 11 | Pending |
| REG-02 | Phase 11 | Pending |
| REG-03 | Phase 11 | Pending |
| REG-04 | Phase 11 | Pending |
| LEDG-01 | Phase 12 | Pending |
| LEDG-02 | Phase 12 | Pending |
| LEDG-03 | Phase 12 | Pending |
| LEDG-04 | Phase 12 | Pending |
| LEDG-05 | Phase 12 | Pending |

**Coverage:**
- v2.0 requirements: 49 total
- Mapped to phases: 49
- Unmapped: 0 ✓

---
*Requirements defined: 2026-11-10*
*Last updated: 2026-11-10 after roadmap creation (phases 5 to 12)*
