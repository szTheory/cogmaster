# Pitfalls Research

**Domain:** Adding Korean mappers, 93C46 EEPROM and `.srm` interop, atomic runner saves, pin-level peripherals (Light Phaser, Sports Pad, Paddle, 3-D glasses), a BIOS boot path, PAL and Japanese FM ledger coverage and a whole-list ledger to a shipped, deterministic, bounded, integer-only C17 SMS/Game Gear core (cogmaster v1.0 at v0.4.0).
**Milestone:** v2.0 "The whole library"
**Researched:** 2026-11-09
**Confidence:** HIGH for pitfalls grounded in this repository's code, budgets and CI (read directly); HIGH for CC0 list facts (fetched at MAME pin `1d7a454ed79e` today); MEDIUM for hardware-behaviour pitfalls (EEPROM timing, phaser sensor, paddle clock), which come from the preparation files, MAME BSD-3 and datasheet summaries and must be settled by each phase's own fixtures.

**Licence note:** No GPL, LGPL, non-commercial or unlicensed emulator source was opened. Genesis Plus GX and Gearsystem behaviour (`.srm` sizes, word order) is listed as unknown and is to be measured from released binaries only (AGENTS rule 1). MAME `hash/*.xml` (CC0) and MAME BSD-3 `sega8/rom.cpp` and `eepromser.cpp` were read at the pinned commit.

## Phase labels used below

The roadmapper decides the real phase list; these labels are the mapping keys. SEED-001's note puts the Light Phaser after the line timing has held for a phase, and SEED-007 puts the tune-up first.

| Label | Content |
|---|---|
| **P1 Tune-up** | v1.0 debt, measured CI timeouts, perf gate armed, review triage, README ROM step, triage digest, `quality.json`, `policy.lessons_unique` |
| **P2 Mappers** | Korean family and other boards with hash-database detection |
| **P3 Saves** | 93C46, `.srm` interop, runner `--save-dir` durability, hosted save round trip |
| **P4 Peripherals** | Light Phaser, Sports Pad, Paddle, 3-D glasses |
| **P5 BIOS** | `--boot bios` and the libretro BIOS option with the user's own dump |
| **P6 PAL and FM** | PAL and Japanese FM over the ledger |
| **P7 Whole ledger** | ledger over the whole CC0 lists, `is_experimental` removal |
| **ALL** | every phase pays it (budgets, timeouts, perf gate, docs) |

## Measured starting position (read from the tree on 2026-11-09)

These numbers drive several pitfalls below; they are facts, not estimates.

| Budget | Cap | Now | Headroom |
|---|---|---|---|
| `docs/testing.md` | 40,960 B | 40,953 B | 7 B |
| `docs/embedding.md` | 40,960 B | 40,385 B | 575 B |
| `docs/conformance.md` / `docs/runner.md` | 40,960 B | 29,621 / 26,095 B | about 11 KB / 15 KB |
| `README.md` | 12,288 B | 3,464 B (71 lines) | 8.8 KB by bytes (the owner calls it capped; confirm which cap is meant) |
| `include/cogmaster.h` | 24,576 B | 8,332 B | 16 KB |
| `tests/retroarch` plus `fixtures/play.h` | 300 lines | 299 lines | 1 line |
| Repository-check code vs `src/` (strict one tenth) | 10 % of 7,444 `.c`/`.h` lines = 744 | 528 counted lines | about 216 lines |
| `tests/perf/allow.txt` | at most 60 days out | `boot_to_play` +3.57 % to 2027-01-07 | gate still report-only |
| CI timeouts | twice slowest of last 40 | 14 jobs still `provisional` on 2 to 7 runs; windows-arm64 7 min on 187 s | none |
| `src/cartdb.inc` | | 1,656 rows, each `char title[256]` plus `char sha1[41]`: about 0.55 MB of const data (derived) | `size.*` ceiling not yet in `quality.json` |

## Critical Pitfalls

### Pitfall 1: The tune-up sets the gates against pre-milestone code, then the milestone trips them

**What goes wrong:** P1 re-derives timeouts, arms the instructions-per-frame gate and builds `quality.json` ceilings from today's tree. P2 to P7 then add ten-odd fixtures, per-board hash lanes on eight legs, a peripheral path in the I/O read hot path and larger fuzz corpora. The first hosted run of each phase head times out or trips the +1.00 % gate, which is exactly v1.0 lesson 1 (timeouts measured before the code existed failed GCC, MSVC, UBSan and macOS x64 and later windows-arm64).

**Why it happens:** SEED-007 puts the tune-up first and says to re-derive from the last 40 runs. Those 40 runs cover code that does not yet have the milestone's load. Most `ci.yml` timeouts are still `provisional` from 2 samples.

**How to avoid:**
- P1 derives timeouts and ceilings, but every later phase's plan carries a task "re-measure the timeout of every lane this phase made slower, on the first hosted run of the phase head" with the run ID in the YAML comment (ENGINEERING 4 wording). Treat `quality.json` ceilings as reviewed-per-phase, not set-once.
- Arm the perf gate in P1 only after two zero-spread control runs on a `main` baseline that includes sound (the audit's wording), and size the baseline ROM set so each new subsystem adds one ROM (a mapper ROM per new board family is wasteful: one Korean-board ROM, one phaser-attached ROM).
- Keep the `boot_to_play` allow line for the 357 (3.57 %) sound cost, but re-baseline it in P1 so the expiry (2027-01-07) cannot land mid-milestone as a surprise red gate. Allow lines are one line each with a reason; do not let them stack.

**Warning signs:** a hosted job cancelled at its timeout on the first run of a phase head; a PR that is red only on `perf` after adding a switch case to `mapper_read`; `provisional` still in a comment after P1.

**Phase to address:** P1 sets; ALL re-measure.

---

### Pitfall 2: Budgets that are already at their cap make every feature plan fail its own docs or test step

**What goes wrong:** AGENTS rule 5 says every behaviour change lands with a test and documentation. `docs/testing.md` has 7 bytes of headroom, `docs/embedding.md` (the natural home of the API and save contract) has 575, the hosted RetroArch code is at 299 of 300 lines, and the strict one-tenth check-code ratio leaves about 216 lines. DEC.63 also promises a hosted RetroArch save round trip in v2.0, which needs lines that do not exist. The first feature plan that touches any of them fails `docs.budgets`, `policy.e2e_budget` or `policy.gate_budget`, and the cheap fix is to cheat (trim real content, move check code into an unclassified directory, or raise a cap without a reason).

**Why it happens:** v1.0 lesson 3: budgets left at their ceiling. The tune-up itself is check-code heavy (digest script, `quality.json` checker, `policy.lessons_unique`, ledger work) and lands first, when `src/` is smallest and the one-tenth limit is tightest.

**How to avoid:**
- First plan of P1: restructure docs. Split `docs/testing.md` by topic (for example CI lanes versus fixtures versus ratchets) and open a new `docs/saves.md` and `docs/peripherals.md` instead of growing `embedding.md`. `docs.budgets` globs `docs/*.md` at 40,960 bytes each, so new topic files are the honest answer; do not move prose under `docs/hardware/` just because the glob does not recurse.
- Put the hosted RetroArch save round trip inside the existing 300 lines by deleting code (shared helpers: `bmp.c` 61 and `compare.c` 61 lines are candidates), in the same plan that adds it (v1.0 lesson 3). Do not raise the cap; DEC.63 and AGENTS rule 7 are settled. Prove semantics in-process or with the runner, and keep RetroArch to the single save round trip.
- Count check-code lines before P1 starts and budget them: about 216 lines for the digest, `quality.json` check, `lessons_unique` and any ledger or audio-predicate changes. Prefer CMake `string(JSON)` and one-pass awk, and implement `quality.json` as one check (DEC.59). Mapper and peripheral phases then raise the allowance by about one line per ten `src/` lines.
- Confirm what "README at cap" means: by bytes it is 3,464 of 12,288. The ROM step (and the BIOS legality sentence) needs maybe 600 bytes; if there is a separate line or section budget, find it before planning.

**Warning signs:** a plan whose docs task says "trim to fit"; a new file under `tests/` that is not in `tests/policy/repository-checks.txt`; `policy.gate_budget` reporting a ratio within 20 lines of the limit.

**Phase to address:** P1 (docs split, e2e line reclaim, check-line budget); ALL (each plan names which budget it pays from).

---

### Pitfall 3: Mapper misdetection: hash table trusted too far, heuristics trusted too little, and a silent change from "refuse" to "play garbage"

**What goes wrong:** Today an image not in the SHA-1 table runs through `other_board()`, which counts raw `0x32` bytes (not instructions) over the first 32 KiB, and answers `UNSUPPORTED_BOARD` when `other > sega`. Once Korean boards exist, the same scan will pick a board instead of refusing. Raw-byte scanning has no minimum count and no instruction-boundary awareness, so data tables containing `32 02 00`, `32 00 A0` or `32 FE 3F` flip a Sega game to a Korean board, and translations, hacks, new dumps and homebrew (never in the CC0 lists) are the images that depend on it. The reverse also occurs: a hash-listed entry whose `slot` is wrong in the list is obeyed absolutely (a hash match "overrides steps 3 to 6").

**Why it happens:** CART section 1.4 places the heuristic after the table and copies MAME's scan; the current code was written when the only outcome was a refusal, so false positives were cheap.

**How to avoid:**
- A valid `TMR SEGA` header with an export or Japanese SMS region nibble and a checksum that passes is Sega-mapper evidence that wins over the scan (Korean boards have no header: CART 1.3). A GG image never selects an SMS-only Korean board.
- Require a threshold and a margin for any heuristic board (for example at least N hits and a ratio over the competing family), tuned on decoy images built from owned bytes: a data table of `32 xx xx` triples must stay Sega; a synthetic Korean-style image must select the Korean board.
- On a tie or weak evidence, refuse with the named candidate board (`UNSUPPORTED_BOARD`), never guess. Report the source of the board (`db`, `header`, `heuristic`) through `cogmaster_probe` and the runner `--probe`, so the ledger can mark heuristic selections as such and a user can tell why a hack misbehaves.
- Add a `--board` style override in the runner only if the owner asks; the library should not grow behaviour hacks keyed by hash (DEC.26: "mapper hints never become behaviour hacks").
- Property test: for every row of the DB, probe of an image with that body hash but the heuristic path forced must either agree with the DB or be flagged; run it from the generator's output so DB drift is caught.

**Warning signs:** a DB-listed Sega game that begins to select a Korean board after a heuristic change; `rom_write` faults at the very start of play (the first-fault capture shows a write to `$4000`, `$6000` or `$A000` on the Sega board); a headerless dump whose board flips when one byte changes.

**Phase to address:** P2.

---

### Pitfall 4: Bank arithmetic written for 16 KiB pages is wrong for 8 KiB boards, and legitimate register writes are reported as faults

**What goes wrong:** `mapper.c` stores `uint8_t bank[3]`, pads the image to 16 KiB pages and applies `bank % pages`. Zemina, Nemesis, Janggun and 188-in-1 select 8 KiB banks; Hicom selects 32 KiB; 4PAK, Janggun and Korean write `$3FFE/$7FFF/$BFFF`, `$4000-$A000` and `$A000`. Reusing `bank_of`/`rom_at` gives half-page or off-by-a-factor reads that still boot a menu. Separately, `bank_write()` records a `MAPPER_RANGE` fault whenever the value reaches past a non-power-of-two image, and the Sega branch records `ROM_WRITE` for any write below `$8000`. Korean boards (Pigu-Wang is 432 KB; the lists also have 368 KB and 800 KB images) hit both paths with legal writes. The first-fault record is the diagnostic of record (DEC.40, XFER-055), so a spurious early fault hides the real one.

**Why it happens:** the Sega board is the template; its fault rules were written for it. Values like 188-in-1's `(value ^ $1F)` base and the Sega bank shift can exceed 255 before reduction.

**How to avoid:**
- Express every board's banks as byte offsets into the padded image computed in `uint32_t` and reduced modulo the image length (or page count of that board's granularity) before indexing; no board may index `rom[]` with an unreduced value. Add an exhaustive test per board: write every value `0..255` to every register, read every address, assert no index outside `[0, size)` (an ASan lane then proves it).
- Decide per board which writes are faults: a write to a documented register is never a fault; a write to an undocumented ROM address keeps `ROM_WRITE`. Test with `cogmaster_get_fault` that the register writes of a clean fixture leave "no fault".
- Keep the dispatch a `switch` on board kind (DEC.37, PIC-safe); the 256-entry bit-reversal table for Janggun must be file-scope `const` data, not a table of function pointers (XFER-071).
- Power-on state per board in `mapper_reset`, cited. Do not extend the "BIOS leaves 0/0/1/2" hand-off to boards whose registers the BIOS never writes.

**Warning signs:** a fault trace beginning with `mapper_range` on a known-good Korean title; `boots@F` passes on a menu but frame hashes differ between 8 KiB and 16 KiB modelling; ASan reports a read past the ROM allocation on the 432 KB or 800 KB image class.

**Phase to address:** P2.

---

### Pitfall 5: Mapper registers settled by fixtures that share the implementer's misreading

**What goes wrong:** The preparation file lists open register facts (Janggun reverse flag: wiki says bit 6, MAME tests `>= $80` and masks 6 bits; 188-in-1 menu entries 6 and 13 fail in both MAME and ares; Seo Jin XORs reads with `$80`; the Nemesis and Hap2000 layouts exist in one reference only). A self-authored WLA-DX fixture can only prove what its author believed; passing it settles nothing, and the ledger then shows `pass` for a board whose register semantics are a guess. The "oracle cannot be fooled" property (XFER-025) is lost when the author of the fixture and the implementation are the same session.

**How to avoid:**
- Treat each ambiguous register as confidence-qualified (DEC.15) with its source named in a code comment, and write the fixture to expose both candidate behaviours so a pass proves one of them.
- Settle with independent evidence: ares (ISC) source may be read for structure, MAME BSD-3 likewise, released binaries run on the owner's own dumps locally (command, exit status, nothing committed). The owner owns Korean carts; one local run settles Janggun bit 6 versus bit 7 more reliably than a fixture.
- Ledger rows for entries that both references fail (188-in-1 entries 6 and 13) are `fail` or `skipped` with the reason, never `pass`. Do not average away disagreement.

**Warning signs:** a fixture comment that cites no reference for the register layout; a Janggun row `pass` with `evidence` pointing at an owned fixture only.

**Phase to address:** P2 (fixtures and confidence labels); P7 (ledger honesty).

---

### Pitfall 6: Implementing the 93C46 as "battery RAM at `$8000`" instead of a bit-banged serial device behind a one-byte window

**What goes wrong:** The Sega-board code paths already treat slot 2 as RAM when `$FFFC` b3 is set (`mapper_read`/`mapper_write`). The EEPROM board is different, as MAME's BSD-3 `sega8/rom.cpp` shows: b3 opens a window only at the single address `$8000` (every other slot-2 address still reads ROM bank 2); a write to `$8000` drives DI (b0), CLK (b1) and CS (b2); a read returns DO in b0, b1 forced to 1 and the rest as last written; `$FFFC` b7 resets the chip (on the plain Sega board b7 is "ROM write enable" and the current code raises a `MAPPER_RANGE` fault for it). Reusing the RAM branch makes the first menu pass and every save corrupt or fail. Further failure modes: no write-protect state (a real 93C46 powers up write-disabled and writes only after EWEN), instant writes with no busy state, a started but unfinished command surviving a reset or state load, and a bit stream that never ends.

**Why it happens:** CART records the `$8000` semantics in one table row and `cart_copy` sizes `ram_size` from the board; the generation counter in `cogmaster__battery_write` is tied to RAM offsets.

**How to avoid:**
- A separate board kind (`eeprom`) with its own read and write branch; the 128 bytes live in instance memory in the exact exported order (64 little-endian words, DEC.67) so `cogmaster_get_battery` can keep returning a stable pointer and length with no conversion at export. Mark them `COGMASTER_BATTERY_KNOWN`; extend `cogmaster_cart_info` append-only.
- Model the chip as a small state machine: start bit, 2-bit opcode, 6 address bits, 16 data bits; READ, WRITE, ERASE, WRAL, ERAL, EWEN, EWDS; power-up write-disabled (datasheet summaries agree for the 93C46 family, MEDIUM; confirm against the datasheet the owner's cart uses); unknown or malformed sequences ignored; CS low aborts. Bound the machine so any bit sequence returns to idle in at most 25 clocks plus the busy interval.
- Busy time is counted in master ticks, never read from a host clock. Datasheet maxima differ (2, 5 and 10 ms appear across vendors, MEDIUM); pick a value, cite it, and make DO read busy until the deadline using the lazy "compare against `m->ticks`" pattern so no new scheduled event is needed. MAME's model uses a timer on machine time; copy the intent, not the mechanism.
- Increment the generation counter only when stored data actually changes (completed WRITE, ERASE, WRAL or ERAL), not on every clock edge; otherwise the runner would flush on every bit.
- Put the shift-register and busy state in the save-state visitor and bump `COGMASTER_STATE_FORMAT_VERSION`; a state taken in the middle of a command must resume exactly (round-trip test mid-command).
- Fuzz target `fuzz.save`/bit-banger: random `$8000` and `$FFFC` traffic, plus a test that the instruction count per frame stays bounded.
- An owned WLA-DX fixture writes a known 64-word pattern, power-cycles (reset with the same instance), reads it back, and checks write-protect and busy through the RAM mailbox; the oracle must be able to fail (try EWDS then WRITE and expect the old word).

**Warning signs:** an EEPROM game that works only if its save is never reloaded; a runner that fsyncs several times per frame during a save; `rom_write` or `mapper_range` faults at the first `$FFFC` write on a baseball game.

**Phase to address:** P3.

---

### Pitfall 7: `.srm` interop guessed rather than measured: size, padding, word order and wrong-game files

**What goes wrong:** Users will point cogmaster at existing `.srm` files from RetroArch cores. CART 4.2 records that the byte count and EEPROM word order written by Genesis Plus GX and Gearsystem are unverified. If another core writes a larger file for an 8 KB game, `cogmaster_import_battery` today returns `ERR_SAVE_SIZE` for any file longer than the span, so the user's saves are rejected, which defeats interop. A short file is silently padded with `$FF`, so a truncated or wrong-game file is accepted. If an emulator stores 93C46 words big-endian, an imported save has the right length and wrecked contents (names and stats byte-swapped), which no size check can catch. Two ROMs with the same stem (`Sonic.sms`, `Sonic.gg`) share one `<stem>.srm` in the runner's `save_path`. `Exact-size wrong-game raw files cannot be identified` is already documented.

**How to avoid:**
- Settle by measurement exactly as DEC.67 and CART 8 say: run the released libretro binaries once on an owned battery cart and an owned EEPROM cart, record the sizes and a word-order probe as a command and exit status (AGENTS rule 11), commit nothing. Do not open the cores' GPL sources to find out.
- Choose the import rule from the result: if the other core writes 32 KB for an 8 KB game, accept longer files only when the excess bytes are a recognisable fill (all `$00` or all `$FF`) and reject otherwise; pad on import, never on export (DEC.67). Keep the transactional "validate then apply" behaviour.
- Until measured, README and `docs/saves.md` must not claim interop with named emulators. State only what is tested ("raw bytes of the on-cart RAM; 128 bytes for 93C46").
- Add the platform tag to the file name in the runner (`<rom>.srm` stays for RetroArch parity; document the same-stem collision) or detect it with a size/`machine` check; state a rule rather than silently overwriting.
- Test word order with a pattern whose bytes are all distinct and assert the exported `.srm` bytes, so the choice cannot flip unnoticed.

**Warning signs:** a user issue saying "my RetroArch save is rejected as oversized"; a save whose text appears byte-swapped; the same `.srm` rewritten by two different ROMs in one directory.

**Phase to address:** P3.

---

### Pitfall 8: Runner save durability: "atomic" ends at the rename, and the flush happens only at exit

**What goes wrong:** `runner/save.c` already writes a temporary beside the target, fsyncs, renames (POSIX) or `MoveFileExW(REPLACE_EXISTING|WRITE_THROUGH)` (Windows) and has fault-injection points. Gaps found by reading it: (1) `cogmaster__save_flush` is called once at the end of `main.c` (and on SIGTERM), so a SIGKILL, power loss or crash during a long run loses every save made since load; (2) there is no directory `fsync` after `rename` on POSIX, so the rename can be lost across a power cut even though the data was synced; (3) a process killed between `open(O_EXCL)` and `rename` leaves `<path>.tmp-N` behind and the 1,024-slot probe loop is never cleaned, so repeated kills eventually fail with "cannot create temporary"; (4) a successful flush acknowledges `b.generation`, but a failed one leaves the dirty state, which is right, so keep it that way when adding periodic flushing; (5) the existing atomic code must not be rewritten for the milestone ("runner `.srm` replacement is atomic on POSIX and Windows" is already in Implemented scope).

**How to avoid:**
- Treat "runner `--save-dir` atomic writes" as hardening plus a hosted round trip, not a rewrite: add a bounded stale-temporary sweep (only names matching the exact `.tmp-N` pattern for this save stem), directory `fsync` on POSIX, and a deterministic periodic flush (every N completed frames when the generation changed; never wall-clock time) so a crash loses at most N frames of saves.
- Keep the fault-injection seams (`COGMASTER_SAVE_*`) and add an EEPROM generation test: many bit edges, one flush.
- Add a SIGTERM-mid-write test using the existing raise-signal seam, assert exit 0 and that the file is old-or-new, never partial.

**Warning signs:** `.tmp-*` files in a user's save directory; a save lost after the process was killed; a test that passes only because the process exits cleanly.

**Phase to address:** P3.

---

### Pitfall 9: Light Phaser built as a read-time TH poll instead of a beam-time edge that drives the H-counter latch

**What goes wrong:** Real software busy-polls `$DD` for TH, then reads the V counter and the H counter that the VDP latched at the moment TH fell (CART 3.3). If the core evaluates the sensor only when the Z80 reads `$DD`, a TH pulse that starts and ends between two polls is lost, the H latch is taken at read time, not at the lit dot, and games such as Gangster Town and Space Gun aim a few pixels off or not at all. MAME keeps a "sensor-on flag until `$DD` is read or the frame ends" and per-game X offsets (CART 3.3: seven games with values 3 to 22) to paper over exactly this; the latch-edge rule (SMS Power: latch while TH is 0; MAME: latch on 0 to 1, flag on 1 to 0) is still an open question in CART 8.

**Why it happens:** the core renders per line with catch-up (DEC.30), and the existing `th_pins`/`io_control` only latches on `$3F` writes. A phaser makes TH a function of the picture, which the I/O layer has never needed.

**How to avoid:**
- Settle the edge rule with one fixture before building features on it: a WLA-DX phaser exerciser that polls TH at several loop lengths and reads the latch, judged through the mailbox. The open question must be closed by a test, not a decision in a plan (CART 8 says the "Color & Switch Test" and Hang-On/Safari Hunt settle it; those are third-party ROMs with no licence statement, so they are local or fetched-lane evidence only).
- Model the sensor as incremental: on each VDP catch-up to dot d, scan only the dots since the last evaluation for the first lit dot inside the radius; on a hit, latch H at that dot's tick (the VDP latch is already dot-exact) and set the pin low for as long as the sensor stays lit. The scan cost is then bounded per line regardless of how often the ROM hammers `$DD`; a hostile ROM cannot multiply the work.
- Define the sensor on the core's own RGB444 field with integer weights (for example 77, 150 and 29 over 256), and the threshold in the same unit; the SMS 2-bit channels give mid-level colours that sit within a step of the `$7F` boundary, so pin boundary cases with a fixture and never claim equality with MAME. Say which rows the radius covers (current line only or plus and minus six lines) and test it.
- Do not encode MAME's per-title offsets as a header-keyed table in `src/`. If a game needs an offset, it is a hardware delay: model one global constant first; if a per-title value is still needed, it is a fact in the generated cart database (DEC.26 forbids behaviour hacks).
- Coordinates: the host gives pixels in screen space, the core maps them to the 256-wide field and the active crop (SMS 192/224/240 rows; GG crop origin 48, 24; SMS left-column blank). Integer arithmetic only, with the libretro pointer range `[-32768, 32767]` scaled by documented rounding in the adapter, not the core. Offscreen is its own state, not clamped.
- Only evaluate the sensor when a phaser is selected on that port; with no phaser attached the hot path must cost nothing (the perf gate checks it).

**Warning signs:** a title that needs the trigger held for several frames to register; shots land a constant offset right of the cursor; the Ir/frame of an ordinary game changes after the phaser lands.

**Phase to address:** P4, after the VDP timing assumptions are stated (a fixture proves the latch positions 600 and 604 are unchanged).

---

### Pitfall 10: Sports Pad and Paddle written in microseconds and wall-clock-shaped logic, then broken by aliasing

**What goes wrong:** The documents quote 80, 40 and 143 µs (MAME: 512 Z80 cycles, which is exactly 1,536 ticks) and 60 µs (ares), and the Paddle toggles on its own 18 kHz clock. Implemented with floating point, host timers or per-frame counters, they break determinism. Worse, a deterministic free-running toggle can alias with the game's own detection loop: games sample TR 256 times and expect about 50 % ones; a toggle period that is an exact multiple of the loop length returns 0 % or 100 % and the game reports "no paddle". Real hardware has jitter that an exact model does not.

**How to avoid:**
- All timings are integers on the tick lattice (1 Z80 cycle = 3 ticks): Sports Pad 80 and 40 µs are about 859 and 430 ticks, the 143 µs reset is 1,536 ticks; the paddle half period at 18 kHz is about 597 ticks (derived; choose one value and cite it). Evaluate timeouts lazily against `m->ticks` at the next TH edge or `$DC/$DD` read, so no deadline dispatcher entry is added and a ROM hammering the port costs O(1).
- Settle the timeout (143 versus 60 µs) with the two titles named in CART 8 (both must track the ball); the owner holds the carts, so this is a local run recorded as command and exit status.
- Test the paddle detection routine as an own fixture at several loop lengths (for example 20 to 60 cycles) and require the ratio of ones to stay inside 30 to 70 % for each; if any length aliases, shift the toggle phase per sample from the tick counter so it is deterministic but incommensurate.
- Motion is relative: the pad accumulates deltas across frames and saturates in a documented way; the runner input script must record deltas, not absolute positions (the existing `runner/input.c` format and `docs/runner.md` need an append-only extension). Losing motion when no game reads a frame makes the cursor drift under TAS replays.
- On a Japanese region the Sports Pad Soccer polls TL/TR instead; Mark III lacks TH. Test the region-dependent path through the existing TH echo rule.

**Warning signs:** a pad-detect loop returning a constant; replays diverging after a frame in which the game did not read the pad; a timing constant containing a decimal point.

**Phase to address:** P4.

---

### Pitfall 11: Peripheral state outside the visitor, the API and the ledger identity

**What goes wrong:** New device state (pad phase and counters, paddle toggle, phaser latch and flag, glasses shutter, EEPROM shift register) lives only in `m->io` or the board and is not in the save-state visitor; load-state then diverges, which breaks run-ahead and the "same inputs, same hashes" claim. `cogmaster_input` is a size-tagged struct with `port1`/`port2` limited to `PAD` (0) and `NONE` (1), and `cogmaster_set_input` rejects `port > COGMASTER_DEVICE_NONE`; adding devices by inserting values or reordering breaks ABI and hosts. A device choice is part of the configuration but not of the ledger key today (the `cfg:` string carries `port1=pad;port2=pad`), so two runs with different devices can collide or be judged by the wrong expectation. Missile Defense 3-D needs both the phaser and the glasses at once.

**How to avoid:**
- Every new state field goes through `state.c` with bounds checks (the visitor validates before applying) and the state format version is bumped once per phase that adds state; add the round-trip test per device in the same plan.
- Append device ids after `NONE` (2 and up), append fields to `cogmaster_input` (`check_size(in, 32, ...)` keeps old callers working), update the `> COGMASTER_DEVICE_NONE` check to a table-driven maximum, and add each to the ABI/API version notes; the header stays terse (prose in `docs/peripherals.md`).
- The ledger `cfg:` prefix must carry the device and script identity (the contract already says it does: extend `compat-common.sh` and the key); add a ratchet test where a changed device is a different key.
- Lag frame: the counter is "a read of `$DC`, `$DD`, `$C0`, `$C1` or GG `$00`"; phaser and pad polling count, but the glasses are write-only and must not.
- Do not hash peripheral internals into the frame hash; the frame hash covers pixels (DEC.33).

**Warning signs:** a state test that passes only when state is saved at frame boundaries; a header diff that renumbers `COGMASTER_DEVICE_*`; two ledger rows that differ only in a device string.

**Phase to address:** P4 (with P7 for the identity).

---

### Pitfall 12: 3-D glasses modelled as a mapper register, with an eye signal the core cannot compose

**What goes wrong:** The glasses snoop writes to `$FFF8` to `$FFFB` (b0 = 0 right open, 1 left open) on the cartridge bus; they are readable only through the work-RAM mirror, so presence is undetectable. `cogmaster__bus_write` sends only `>= $FFFC` to the mapper, and only if `$3E` has the cartridge enabled; a naive `$FFF8` hook that ignores `$3E` sees writes the adapter never would, and one placed in the mapper misses games that run with another board. The shutter takes about 10 scanlines to switch (MacDonald via MAME), so "eye per frame" is wrong for games that toggle mid-frame. Composing both eyes (anaglyph) needs two retained fields, which the core's fixed one-field buffer and the zero-allocation rule do not give for free. Blade Eagle, Maze Hunter and Missile Defense need glasses; the rest have 2-D modes.

**How to avoid:**
- Hook the bus write path once, gated by `cart_on` and a "glasses attached" flag, store the eye and the tick of the last change, and report the eye per rendered frame sampled at a stated point (frame end), with the mid-frame change documented as not modelled until a fixture needs it (like the existing "mid-frame input not modelled").
- Keep composition in the host: the core exposes the eye flag; the libretro adapter chooses left-only, right-only or alternate output (the default must show a playable image, not flicker); an anaglyph helper belongs to the host helpers outside the hash (DEC.33). Do not add a second field buffer to the core unless a measured need appears.
- The hot-path cost of an extra address test in `bus_write` is on every store: keep it a single compare behind the existing `addr >= 0xC000` branch and check Ir/frame.
- Note the model is the SMS1 card slot or Japanese jack only; SMS2 and GG cannot use it, so the option must be refused or inert there rather than silently on.

**Warning signs:** an eye flag that changes while a ROM is run with the cartridge disabled; frame hashes that differ between runs of the same script because the sample point moved.

**Phase to address:** P4.

---

### Pitfall 13: The BIOS path: legality, test design and slot arbitration

**What goes wrong:**
- **Legality.** ASSET_POLICY forbids any BIOS byte, frame of the BIOS logo, state, RAM dump or recording made while running a real BIOS in the repository or release. `release.firmware_scan` and `local.firmware_window_scan` are described as "planned" and exist nowhere in `tests/`, `scripts/` or `cmake/` (checked), and no digest table is in the source. A BIOS feature merged before those guards has no automated stop. A BIOS-running state file also holds a copy of BIOS code in work RAM, because the BIOS copies itself to RAM.
- **Testing without a BIOS.** The path is "recognised by digest", but CI has no BIOS; a test that skips when the BIOS is absent is forbidden (XFER-011), and a test hook that accepts any bytes becomes a product loophole.
- **A fetch lane.** CONFORMANCE section 3 allows fetching third-party ROMs at a pin; extending that idea to a BIOS (from an archive, a libretro system bundle) is the tempting mistake and is prohibited.
- **Behaviour.** The BIOS and cartridge are both on the bus under `$3E` control; on SMS2 their outputs are ANDed when both are enabled (SMS-SYSTEM section 3). Modelling the BIOS as an exclusive overlay is subtly wrong. Export BIOSes check the header and checksum, so many Japanese, Korean and GG cartridges that boot fine under fast boot lock up under `--boot bios`. Turning BIOS on "automatically when found" (libretro system directory) regresses compatibility for users who merely have the files.
- **States.** A state must record which BIOS (digest) it was made with, or loading under another BIOS hangs inside bounded time but wrongly.

**How to avoid:**
- Order inside the phase: (1) the two firmware scans and a digest-table test with synthetic negative controls land first; (2) the core API takes BIOS bytes as media of a documented size and role and validates only size and bounds, while the digest recognition and the "unknown BIOS digest: reported by name" message live in the runner and adapter; (3) the CI evidence is an owned BIOS-shaped image assembled from MIT WLA-DX source (8 KB, `$3E` hand-off, copy-to-RAM, jump) that proves the mechanism: slot arbitration, the AND rule, and the hand-off equal to the fast-boot state (DEC.43).
- Results of runs with a real BIOS are local (`library.local`, `COGMASTER_BIOS_DIR`), never in the public ledger beyond a single boolean, never in CI artifacts, never as a committed hash of a BIOS frame without the policy's reading (ASSET_POLICY section 2 allows single facts; keep them local by default).
- Default stays fast boot (DEC.24); `--boot bios` and an explicit libretro option opt in, both named in the README in one sentence with no link to where BIOS files may be found.
- A state header field carries the BIOS digest or a "none" marker.

**Warning signs:** a plan with the word "skip" next to BIOS; a test whose input path is built from `COGMASTER_BIOS_DIR`; a wget or `gh release download` of anything ending `.rom` or `.bin` in a workflow; a frame in `docs/` that shows a Sega logo.

**Phase to address:** P5 (guards first); README sentence in P1.

---

### Pitfall 14: PAL covered by a profile that exists, but nothing proves it, and no field says which cartridges are PAL

**What goes wrong:** `instance.c` already has the PAL tick budget, the VDP has PAL geometry and `docs`/the performance budget list a PAL profile, but the ledger has zero PAL rows. The CC0 lists carry no TV-system field (verified by listing every `feature` and `sharedfeat` name in `sms.xml` and `gamegear.xml` at the pin: `pcb`, `ic*`, `slot`, `battery`, `pin_42`, `ctrl1_default`, `ctrl2_default` and chip names only). A DB-derived "PAL" flag therefore has to come from description text (`Europe`, `Australia`), and Brazil (PAL-M, which counts as NTSC for TV detection, SMS-SYSTEM) and `Europe, Brazil` titles make the text unreliable. Differences that bite: the V-counter jump tables (PAL 192, 224, 240 differ from NTSC), the Pause NMI landing at line 311 instead of 260, audio samples per frame (rational 13,300,856/267,615 Hz versus NTSC), the PSG at the PAL clock, line and frame interrupt cadence, the libretro `fps` conversion (a `double` only at the adapter, never in the core), and TV-detection loops (about 1,924 iterations NTSC, 2,288 PAL, threshold 2,048). The ledger `region` column is `ntsc-u|ntsc-j|pal|any`, which fuses region and TV, so `export + NTSC` and `export + PAL` and FM cannot be told apart.

**How to avoid:**
- Do not invent a PAL-by-title rule. The core already selects TV from the host profile; the ledger rows for PAL are fixtures first (an owned PAL `vdp_lines`, `ports` and audio fixture plus a TV-detection loop fixture that counts both ways), then a small set of real PAL carts the owner holds, judged locally.
- Add PAL fixtures to the eight-lane hash equality (be32 and wasm32 included): PAL audio per-frame sample counts and V-counter tables are the places lanes diverge.
- Split or document the ledger axis before adding rows (`tv` distinct from `region`), as one migration with a ledger format note; the ratchet key includes resolved TV, so do it before P6 rows exist.
- Add a PAL profile to `tests/perf/budgets.txt` data (20,120 µs per frame) and measure.

**Warning signs:** a ledger `pal` row with no PAL fixture beside it; a PAL title running at NTSC speed in RetroArch because the host guessed; sample count per frame not matching the exact rational.

**Phase to address:** P6.

---

### Pitfall 15: Japanese FM over the ledger enshrines unmeasured constants and has no audio predicate to judge with

**What goes wrong:** The ledger grammar has `boots`, `frame`, `ram` and `mailbox`; its text says "Audio awaits phase 4", and phase 4 shipped without an audio predicate. FM-capable titles must be judged with FM on (Japanese profile, `fm=unit`); on an export profile FM writes are ignored, so a row without `fm` in its identity can pass for the wrong reason. The FM:PSG mix (4096:1434 Q12) and SMS2 clipping are provisional (DEC.36; the open question is a hardware recording), so any golden PCM hash recorded from the owner's FM game freezes a provisional mix as "expected". Games detect FM through `$F2`/`$F0-$F1` probing with the I/O chip disabled; the existing code treats that probe as no fault, and ledger smoke must not count it as unmapped access.

**How to avoid:**
- Add an `audio@F=<transition hash>` predicate in the shared sh parser with the transition stream (tick, source, level) as the evidence, because it is mixer-independent (DEC.34); PCM hashes are secondary and must not be the ledger oracle while the mix is provisional.
- Use own fixtures (`fm_sweep`, `psg_sweep`) as the audio oracle and use real FM games only for `boots`-class smoke with FM on, labelled as such.
- Carry `fm` in the ledger configuration identity (profile string), and add a ratchet test where changing it is a different key.
- Keep every provisional audio constant visible in the doc that the ledger links to, so a green row never reads as "sounds right".

**Warning signs:** an audio golden value taken from a game's current output ("no current output becomes its own golden value" is already ledger policy); an FM row with no `fm` in its config.

**Phase to address:** P6.

---

### Pitfall 16: Ledger claim inflation across the whole CC0 lists

**What goes wrong:** Seeding 1,690 rows (653 SMS, 823 GG, 214 SG-1000 per the lists; the code's `cartdb.inc` has 1,656) and then removing `is_experimental` gives the impression that all of them play. Specific mechanisms found in the tree:
- `boots@F` is a smoke predicate: HALT is healthy, two distinct crops suffice, and a game stuck in an error screen that blinks passes; its own text says "may reject static screens" and "No playable, completable or sound claim".
- The ratchet allows only `unknown to anything`, `fail to pass` and `pass to pass` (`compat-common.sh` line 81). A row seeded `unsupported` (for example a Korean board not yet implemented) cannot become `pass` without an allow-list line, and allow lines are for passing base cases with a 60-day expiry. Seeding `unsupported` for what P2 to P6 will implement guarantees a ratchet failure at each phase.
- `docs/compatibility.md` is generated and lists every row (about 460 bytes per row today, 2,316 bytes for 5 rows). 1,690 rows would be about 780 KB and breach the 40,960-byte `docs/*.md` budget; `policy.compat_doc_fresh` would also regenerate it on every run.
- Whole-list rows cannot be run in public CI (no ROMs), so `pass` for a CC0 entry only ever comes from the owner's local library; `compat-update.sh` correctly discards private identity but the public count then mixes owned-fixture passes and locally judged passes.
- Entries that need an unsupported device (Sega Graphic Board is `ctrl1_default="graphic"`; X-Terminator, Master Gear and the SG-1000/SC-3000 boards in the CC0 lists) must be named `unsupported`, not `unknown`.

**How to avoid:**
- Define the `is_experimental` exit as a command over the ledger that cannot be satisfied by volume alone: every row in the lists has a status other than `unknown`, each `unsupported` row names its missing device or board, and a stated minimum of passes on a stronger predicate than `boots` for the owner's collection. Do not name a count that needs a published release (AGENTS rule 8).
- Seed rows as `unknown` and extend the allowed transitions only for `unknown` (already free). Teach the ratchet `unsupported to pass` explicitly if rows must start as `unsupported`; do it in P7's ratchet change, with a negative control, rather than relying on allow lines.
- Generate `docs/compatibility.md` as aggregates (counts by machine, mapper, status and predicate) plus the evaluated rows only; the full seed list stays in the CSV, outside the docs budget.
- Keep two numbers in every summary: "listed" and "evaluated by an accepted predicate", with the second never rounded into the first.
- Process the CSV in one awk pass; a per-row `grep` in sh would be slow on Windows runners (note `compat-common.sh` runs on every leg).

**Warning signs:** a PR that removes `is_experimental` with the ledger still mostly `unknown`; `docs.budgets` failing on `docs/compatibility.md`; a ratchet failure titled `illegal transition` on a seeded `unsupported` row.

**Phase to address:** P7; seeding conventions in P2.

---

### Pitfall 17: Hash-database growth and the generator

**What goes wrong:** `cartdb.inc` rows hold `title[256]` and a 40-character hex SHA-1 plus nine `uint32_t` fields (about 0.55 MB of const data); adding board kinds, device hints, a TV field, `slot` strings and EEPROM facts widens every row. The wasm32 and handheld targets care about size, and there is no `size.*` ceiling yet. The generator (`tools/cartdb.c`) converts MAME XML pinned at a commit; a pin bump or an XML shape change (for example `batt` blank or `??` markers) silently reclassifies entries. A public API field widening (`cogmaster_cart_info`) is append-only but still an ABI statement.

**How to avoid:** keep titles out of the in-core table if size matters (store a title only in the host-side projection used by the ledger), or measure and put a `size.wasm` and `size.<zip>` ceiling in `quality.json` before extending rows; generate board names from MAME `slot` values through an explicit mapping table with a test that every `slot` string in the pinned lists is mapped or explicitly unsupported; fail the generator on an unmapped value. Pin bumps are PRs with a diff-of-classification summary.

**Warning signs:** a pin bump whose diff reclassifies dozens of rows without a note; the wasm artifact growing by hundreds of KB.

**Phase to address:** P2 (generator mapping), P1 (`size.*` ceilings).

---

### Pitfall 18: Hostile ROM bounds regress as new devices add state machines

**What goes wrong:** Every new device is another parser of ROM-controlled bytes: the EEPROM bit stream, mapper writes at any address, Sports Pad and phaser port traffic, and BIOS media size. DEC.39 and AGENTS rule 4 require every call to return within bounded emulated time; a stuck EEPROM busy interval, an unbounded retry loop in a device model, or a sensor scan proportional to read count would break that.

**How to avoid:** device models are pure functions of `(state, tick, input)` with O(1) or per-line-bounded work; no device schedules unbounded events; add each to the `fuzz.run` target (random bus and port traffic with each device attached; the list in CART section 6 names mapper traffic and the 93C46 bit-banger); keep fuzz targets report-only until green for ten runs and triage harness failures as test bugs (XFER-049). Hostile `.srm`: exactly the span or shorter (pad) or reject; EEPROM 128 bytes; BIOS media length validated before copy.

**Warning signs:** a device loop containing `while` with a condition on emulated state that the ROM controls; run time per frame that depends on the number of port reads.

**Phase to address:** P2, P3, P4, P5 each (fuzz extension in the same plan).

---

### Pitfall 19: Fixture sprawl and shared-misreading across ten boards and four peripherals

**What goes wrong:** Each board or device wants its own WLA-DX fixture; v1.0 fixtures are built on every leg and compared across eight lanes, so ten more images multiply build time, `hashes.txt` rows and docs bytes. XFER-050 and 056 warn that fixture provenance consumed phases elsewhere.

**How to avoid:** one parameterised source per family (a single `mapper_probe.s` assembled with `.define BOARD` for the Korean family) writing the mailbox in the same layout as `mapper_sega`; frozen pins in text files like the existing fixtures; no third-party ROM committed. The owner's own dumps judge real behaviour locally. Keep the per-PR fixture frame budget in mind (v1.0: 4,800 frames for 8 images).

**Phase to address:** P2, P3, P4.

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Reuse the Sega RAM window code for the EEPROM | A day saved | Corrupt saves, wrong `$FFFC` b7 meaning, rework of the generation counter | Never |
| Per-title X offsets for the Light Phaser keyed on header bytes | Matches MAME's aim on seven games | Behaviour hacks the DB rule forbids; masks a latch-timing bug | Only as a fact in the generated cart database after a global constant fails, with the evidence |
| Seed unlisted rows as `unsupported` | Honest labels at seeding | Ratchet rejects `unsupported to pass` at each later phase | Never until the ratchet allows the transition |
| Raise the 300-line e2e cap or the 40 KB doc cap | Unblocks one plan | Rule and DEC.63 erode; the next phase asks again | Never; reclaim lines or split docs |
| Flush saves only at exit | Zero code | Lost saves on kill or crash | Library demos only; not for `--save-dir` |
| Re-pin state and frame hashes after a state version bump without an independent oracle | Green CI | A real regression is enshrined as golden | Only when the diff is explained by named new state fields |
| Keep the full 1,690-row list in `docs/compatibility.md` | Complete page | 780 KB doc, budget failure | Never; aggregate and list evaluated rows only |
| Time device behaviour in "microseconds" constants | Matches datasheets | Floats or host time creep in; replays diverge | Never; convert once to ticks with the derivation in a comment |
| Guess `.srm` size and word order from memory | Faster first draft | Silent corruption of user saves | Never; measure released binaries |
| Skip the `size.*` ceilings | Faster tune-up | Unbounded growth of the DB and wasm | Only until the first measured values exist in P1 |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| libretro `RETRO_MEMORY_SAVE_RAM` | Exporting a converted copy for EEPROM, or a size that changes between calls | Stable pointer into instance storage already in `.srm` order; size fixed at load |
| libretro device selection | Treating a controller-port device change as an API call that does not affect state | Keep the device in configuration and in the ledger/state identity; refuse changes mid-run or reset |
| libretro lightgun/mouse | Normalising pointer coordinates with floats in the core | Adapter scales to integer pixels; core takes integer cursor coordinates |
| libretro `retro_get_system_av_info` for PAL | Rounding the PAL rate to 50 Hz | Exact rational converted to `double` only in the adapter |
| RetroArch BIOS lookup | Auto-enabling the BIOS whenever `bios_*.sms` exists | Explicit option; default fast boot (DEC.24, DEC.62) |
| RetroArch save directory vs runner `--save-dir` | Same stem for two systems | Document the collision; check `machine` and size before importing |
| MAME CC0 lists | Reading `description` text as data (Europe means PAL) | Use only the verified fields (`slot`, `battery`, `pin_42`, `ctrl*_default`); everything else is a labelled inference |
| ares and MAME source | Copying register behaviour verbatim | Read for structure, write own code from the cited fact, keep `PROVENANCE.md` notice rules |
| Released reference binaries | Using their output as a golden value | Locators only; a mismatch opens a question, not a fix (DEC.48) |
| CI `ledger` step | Running a local-library result in CI | `ci*` presets never register library tests (exact inventory) |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Extra branch in `bus_write` and `io_in` for devices | Ir/frame creeps by 0.3 to 1 % per feature | One `m->devices` flag test behind an existing branch; measure per PR on the gate's ROMs | The +1.00 % gate arms in P1 |
| Mapper `switch` grows to 15 boards | `mapper_read` per instruction fetch slower for every game | Put the common Sega and ROM-only cases first, keep each case branch-light; the perf ROM set includes a Sega game | Gate arms |
| Phaser sensor scan per `$DD` read | Hostile ROM multiplies work | Incremental per-catch-up scan, bounded per line | Frame budgets on handhelds |
| 1,690-row ledger via per-row `grep` in sh | CI minutes on Windows legs | One awk pass; test on the dash and Git-for-Windows shells | First full ledger run |
| New per-leg fixtures | Windows arm64 passes only because it was given 7 minutes | Re-derive timeouts per phase; share a build across tests | The first hosted run of each phase head |
| `cartdb.inc` row growth | Larger wasm and zip | `size.*` ceilings, titles outside the core | After adding device and TV fields |
| Hosted RetroArch save round trip | Slow Mac runner plus the 300-line cap | Keep it one case, reuse the existing replay helpers | The phase that adds it |
| Fuzz corpora | `tests/fuzz` capped at 524,288 bytes | Seed corpora small; generate in-process | P3 to P5 |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| BIOS fetched or committed for tests | Copyright takedown; public history poisoned (ASSET_POLICY section 7) | Synthetic BIOS-shaped fixture; guards (`release.firmware_scan`, `local.firmware_window_scan`) land first; no fetch lane |
| BIOS-run states, RAM dumps or frames kept as artifacts | The BIOS copies itself into RAM; the logo is protected | Local only; hygiene and scans reject; never uploaded |
| ROM-derived hashes of unlisted files written to the ledger | Leaks private collection identity | Keep existing rule: only SHA-1s public in the CC0 lists leave the machine |
| Save path built from a ROM name | Path traversal or clobbering of another file through the stem | The runner takes the base name only; keep rejecting empty stems; test with `..` and separators on Windows |
| `.srm` import of untrusted size | Memory growth or partial overwrite | Validate length first, then apply (current rule); never parse contents |
| Stale `.tmp-N` files containing save bytes | Privacy and disk clutter | Bounded sweep of the exact pattern |
| GPL emulator source consulted to "just check" a size | Licence contamination (AGENTS rule 1) | Run released binaries only; record command and exit status |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| README says how to play but not how to load a ROM | New users stop at the test card (open v1.0 debt) | Add the ROM step in P1, one or two lines |
| README names a BIOS source | Encourages copyright infringement; legal exposure | Say only that the user supplies their own dump and which file names are read |
| Phaser aims wrong in RetroArch because pointer-to-crop mapping ignores GG crop or SMS blank column | "Light gun broken" reports | Fixture for the mapping with both machines; document offscreen behaviour |
| Glasses titles show alternating flicker by default | Unplayable out of the box | Default to a single eye; offer alternate and anaglyph as options |
| `--boot bios` locks up on cartridges with wrong headers | Looks like emulator fault | Say in docs that BIOS enforces the header check; default is fast boot |
| An imported `.srm` rejected with only an exit status | User does not know why | Message names expected and found sizes (no hashes of the file) |
| Compatibility page says "Experimental" forever or drops it too early | Mistrust or over-promise | The exit condition is a command (Pitfall 16) |

## "Looks Done But Isn't" Checklist

- [ ] **Mapper family:** a DB-listed Korean game boots, but also test an unlisted headerless image and a Sega game with decoy `32` bytes; verify `board_source` and that no `mapper_range` or `rom_write` fault is recorded by a documented register write.
- [ ] **EEPROM:** a title saves once; verify power-cycle readback, write-protect after EWDS, mid-command state save and load, one generation increment per completed write, and the `.srm` bytes against the measured word order.
- [ ] **`.srm` interop:** works with the core's own files; verify against files written by the released libretro cores for the same title (local, no commit) and record sizes.
- [ ] **Atomic writes:** the rename is atomic; verify directory fsync, stale temp sweep, kill mid-run (not only SIGTERM) and periodic flush.
- [ ] **Light Phaser:** a pointer click fires; verify edge-driven H latch with the poll loop at three lengths, both machines' crop mapping, offscreen, and zero Ir change with no phaser attached.
- [ ] **Sports Pad and Paddle:** the game sees movement; verify paddle detect ratio at several loop lengths, delta persistence across frames, and region-dependent TL/TR behaviour.
- [ ] **3-D glasses:** the eye flag changes; verify it is gated by `$3E`, unaffected on SMS2 and GG, and that the default host output is playable.
- [ ] **BIOS path:** boots with the user's BIOS locally; verify no BIOS bytes or BIOS frames exist in the tree or any artifact, both scans exist and fail on their negative controls, and a synthetic BIOS-shaped fixture runs in CI without skip.
- [ ] **PAL:** runs at the right rate; verify V-counter jump, Pause at line 311, exact audio samples per frame and all eight lanes hashing equal.
- [ ] **Japanese FM:** FM plays; verify the ledger row carries `fm`, the audio predicate uses transition hashes, and the export profile ignores FM.
- [ ] **Whole ledger:** row count matches the lists; verify `unknown` is still reported separately, `unsupported` rows name a reason, the generated doc is under 40 KB, and the exit condition for `is_experimental` is a command.
- [ ] **Tune-up:** gates armed; verify timeouts no longer say `provisional`, the perf allow line is re-based, review rows WR-02 and WR-04 are closed or seeded, and each remaining open row has a disposition.
- [ ] **Docs and tests:** every behaviour change has a test in `cmake --workflow --preset ci` and its doc in a file under budget; verify `docs.budgets`, `policy.e2e_budget` and `policy.gate_budget` pass with margin.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Wrong board chosen for a class of unlisted images | LOW | Raise the heuristic threshold, add the decoy image as a regression test, bump behaviour revision, regenerate affected ledger rows |
| EEPROM word order wrong after release | MEDIUM | Fix export order, add a migration note in `docs/saves.md`, accept both orders on import only if a signature distinguishes them, otherwise document; bump battery version |
| `.srm` oversize rejection blocks users | LOW | Relax import to tolerate recognisable fill, release, add the measured size to the test |
| Perf gate red from a device branch | LOW | Move the branch behind the existing flag; if justified, a dated allow line with reason |
| Timeout kills a hosted lane | LOW | Re-measure from the failing run, write the new value with the run ID; never raise blindly |
| Budget breach (docs, e2e, check lines) | MEDIUM | Split a doc by topic, delete duplicate helper code, or reclassify honestly; do not raise caps |
| BIOS bytes reached a commit | HIGH | Stop, rewrite the branch before pushing (ASSET_POLICY section 7); if pushed, history rewrite and a support request |
| State format bump re-pins hashes wrongly | MEDIUM | Re-derive with the independent oracle for each changed fixture and record the cause in the plan summary |
| Ledger over-claim published | MEDIUM | Revert the `.info` change and the README claim in a `fix(docs)` PR; keep the ratchet as is |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| 1 Gates set against pre-milestone code | P1; ALL | Each phase head's first hosted run has no `provisional` timeouts on lanes it slowed; perf gate green with two zero-spread control runs |
| 2 Budgets at their caps | P1; ALL | `docs.budgets`, `policy.e2e_budget`, `policy.gate_budget` pass with at least 5 % margin after P1 |
| 3 Mapper misdetection | P2 | Decoy-image and unlisted-image tests; `board_source` reported; DB agreement property test |
| 4 8 KiB bank math and false faults | P2 | Exhaustive register-value test per board under ASan; no-fault test for documented writes |
| 5 Fixtures share the author's misreading | P2; P7 | Each ambiguous register cites two references or one local owned-cart run; both-fail rows are `fail` or `skipped` |
| 6 EEPROM as RAM | P3 | Owned EEPROM fixture with write-protect, busy and power-cycle readback; mid-command state test; bounded-bit-stream fuzz |
| 7 `.srm` interop guessed | P3 | Measured sizes and word-order recorded as commands; import tolerance test; README claims only measured interop |
| 8 Save durability | P3 | Fault-injection and kill tests; directory fsync present; periodic flush test by frame count |
| 9 Light Phaser timing | P4 | Edge-driven latch fixture at several poll lengths; crop-mapping fixture; zero Ir change when absent |
| 10 Sports Pad and Paddle determinism | P4 | Tick constants only; paddle detect ratio 30 to 70 % at several loop lengths; delta replay test |
| 11 State, API and ledger identity | P4; P7 | State round trip per device; header diff is append-only; ledger key includes device |
| 12 3-D glasses | P4 | `$3E`-gated hook test; default output playable; SMS2 and GG inert |
| 13 BIOS legality and design | P5 (guards first) | `release.firmware_scan` and `local.firmware_window_scan` with negative controls exist before any BIOS code; synthetic BIOS fixture runs in CI |
| 14 PAL | P6 | PAL fixtures in all eight lane hashes; ledger separates TV from region |
| 15 FM and audio predicate | P6 | `audio@F` transition-hash predicate in the parser with a negative control; `fm` in row identity |
| 16 Ledger claim inflation | P7 | `is_experimental` removal gated by a command; listed versus evaluated counts; compat doc under budget |
| 17 DB growth and generator | P1 (`size.*`), P2 (mapping) | Generator fails on unmapped `slot`; size ceilings in `quality.json` |
| 18 Hostile bounds | P2 to P5 | `fuzz.run` covers every attached device; bounded-time test per call |
| 19 Fixture sprawl | P2 to P4 | One parameterised source per family; per-PR fixture frames within the measured budget |

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|-------------|---------------|------------|
| P1 review-debt triage | Triage grows into new requirements (AGENTS rule 7) | Every open row gets fix, seed or close; tooling findings become seeds; at most two fix rounds |
| P1 weekly digest and `quality.json` | The tune-up spends the 216-line check budget | Write them as one awk or CMake `-P` file each; count lines before starting |
| P1 README ROM step | README grows a BIOS how-to | One sentence, no sources |
| P2 research | Janggun flag and 188-in-1 entries unresolved | Plan a local owned-cart run and a both-candidates fixture |
| P3 research | `.srm` sizes and word order unknown | Plan the measurement task first, not last |
| P4 research | Phaser edge rule and sensor window | Plan the exerciser fixture first; keep the 600/604 latch ticks unchanged |
| P5 research | Which BIOS variants and memory maps | Start from SMS-SYSTEM section 6 and the digest table in ASSET_POLICY section 4; do not touch bytes |
| P6 research | No TV field in the CC0 lists | Fixtures first, local owned PAL carts second, no title-text rule |
| P7 | Volume is mistaken for evidence | Gate `is_experimental` on a command over statuses and predicates |

## Sources

- Repository (read 2026-11-09, HIGH): `.planning/PROJECT.md`, `RETROSPECTIVE.md`, `LESSONS.md`, `preparation/README.md`, `SMS-CARTRIDGES-AND-PERIPHERALS.md`, `DECISIONS.md`, `CONFORMANCE.md` section 5, `ENGINEERING.md` sections 4, 7, 8 and 11, `SMS-SYSTEM.md` sections 3 and 6, `ASSET_POLICY.md`, seeds SEED-001 and SEED-007; `src/mapper.c`, `src/cart.c`, `src/battery.c`, `src/boot.c`, `src/io.c`, `src/bus.c`, `src/cartdb.c`, `include/cogmaster.h`, `runner/save.c`, `libretro/libretro.c`, `tests/cmake/budgets.cmake`, `tests/cmake/repository.sh`, `scripts/compat-common.sh`, `tests/compat/ledger.csv`, `tests/perf/allow.txt`, `.github/workflows/ci.yml`, `docs/compatibility.md`.
- MAME `hash/sms.xml` and `hash/gamegear.xml` at `1d7a454ed79e` (CC0-1.0 header confirmed, fetched 2026-11-09, HIGH): `slot` values, 13 `ctrl1_default="lphaser"`, 4 `paddle`, 3 `sports` and 1 `graphic` entries in `sms.xml`, 52 `pin_42` and 7 `eeprom` slot entries in `gamegear.xml`, no TV-system field.
- MAME `src/devices/bus/sega8/rom.cpp` and `src/devices/machine/eepromser.cpp` at `1d7a454ed79e` (BSD-3-Clause header confirmed, HIGH for the described behaviour of that implementation, which is not hardware evidence): the EEPROM window at `$8000`, DI/CLK/CS bits, `$FFFC` b7 reset, timed ready.
- 93C46 datasheet summaries from vendor datasheets (search results, MEDIUM): power-up write-disabled, EWEN/EWDS, self-timed write with ready/busy, vendor write-cycle maxima of 2, 5 and 10 ms; the cart's actual part is not identified.
- Sibling lessons used as hypotheses and applied only where the mechanism matches (LESSONS.md): XFER-011, 012, 013, 019, 025, 044, 049, 050, 051, 055, 056, 058, 071.
- Genesis Plus GX and Gearsystem: not consulted (GPL and non-commercial); their `.srm` behaviour is an open measurement.

---
*Pitfalls research for: cogmaster v2.0 "The whole library" (SMS and Game Gear emulator core in C17)*
*Researched: 2026-11-09*
