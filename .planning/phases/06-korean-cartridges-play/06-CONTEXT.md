# Phase 6: Korean cartridges play - Context

**Gathered:** 2026-11-10
**Status:** Ready for planning (adversarial pass applied: 6 blocking items fixed)

<domain>
## Phase Boundary

The remaining mapper boards load and play with no setting; detection is database-first and never turns a Sega game into a Korean board; unsupported boards say why; no shipped frame or audio hash moves. Requirements MAP-01 to MAP-08.

Method: four role-lens advisors (generator/data format, detection/security, mapper hardware, API/perf/process) plus an adversarial pass (owner's fan-out boilerplate; memory: discuss-fanout-and-ci-efficiency). Reference sources: MAME BSD-3 `sega8` files at pin 1d7a454ed79e read through `gh api` only; no GPL/LGPL source opened.
</domain>

<decisions>
## Implementation Decisions

### Hash table and generator (MAP-01, MAP-02)
- One static slot table in `tools/cartdb.c`: `{slot name, stable slot id, board constant or UNSUPPORTED, reason slug}`. Board numbers come from a shared constant header, not literals. A slot cannot be both mapped and unsupported.
- Scan every `<feature name="slot">` of every `<software>` before any multipart exclusion; an unmapped slot exits 1 with `unmapped slot <name>`; stderr prints `cartdb: slot <name> seen=N emitted=M -> <board>` sorted by name. `--list-slots` lets `generator.cmake` build an authored XML from the table and a second one with `zz_new` that must exit 1 (negative control via `expect_fail.cmake`, tolerant regex).
- Wave-1 rule: plan 1 keeps every ext-board slot at board 0 with a reason; the plans that build a board flip its rows and regenerate (a table row must never name a board whose arm does not exist yet). Plan 1 is split (generator + catalog; header/size ceiling/pin check) to stay at most 10 files each.
- Unsupported-by-design entries in the same table: seojin, korean_188in1, the seven sg1000 slot values (cardcatcher, castle, dahjee_typea, dahjee_typeb, level3, othello, terebi) and the GG mgear and xterminator, each with a reason ("menu entries 6 and 13 fail in MAME and ares" for 188-in-1). The dead `rom` and `sega` arms are removed.
- Row layout, whole bytes, no bitfields: 20-byte SHA-1 binary, u32 bytes, u16 ram_bytes, u8 board, slot, machine, region, video, persistence, geometry, ctrl1, ctrl2, flags (36 bytes, no padding). `_Static_assert` on sizeof and offsets and `sizeof` of the table under 100,000 in `cartdb.c`. Designated initializers; binary search over `memcmp` of 20 bytes (hex sort order equals binary order). C source with integer constants only (endian-neutral on be32 and wasm32).
- No pointer tables in `src/` (`abi.globals` rejects nm types B, b, C, D, d and a PIC `static const char *const x[]` lands in `.data.rel.ro`): slot and reason names live in one `const char` blob with offsets or fixed-width char rows. The named-unsupported id is generated from the slot table, not a hand-written list; it covers Janggun, Seo Jin, 188-in-1, the sg1000 slots, mgear and xterminator.
- Titles leave the table: `tests/compat/catalog.tsv` (sha1, machine, board name, slot, title; public rows only) is written by the same generator run; `scripts/compat-common.sh` update mode reads it instead of grepping the `.inc`. `cogmaster_cart_info.title` and `--probe title=` become empty on a hit; documented in `docs/boards.md`.
- Populate persistence (EEPROM from `slot == eeprom`, since those rows carry no battery feature) and ctrl1/ctrl2 from `<sharedfeat>` now, with generator-test count assertions and no library reader (Phases 7 and 8 consume them); no hash moves.
- `size.cartdb` in `quality.json`: the first value is 102400 (the MAP-02 hard bound) with a `reasons` entry naming MAP-02, because a hosted measurement cannot exist before the PR that adds the key; tighten to measured plus about 15 % in a follow-up after the first hosted run. Measured on Linux x64 only in the existing "Size ceilings" step; the in-source `sizeof` static assert guards every lane that compiles `cartdb.c`. Add a check that the `.inc` header's pin equals the `pins.txt` MAME version. The pin does not change.

### Detection and probe (MAP-06)
- `guess_board()` replaces `other_board()`. Order: database SHA-1 hit; Codemasters header (kept); valid `TMR SEGA` header (magic, region nibble 3 to 7, checksum_ok, range within body; a GG-region header never selects an SMS-only Korean board); size rule (at most 48 KiB is ROM-only; 8 KiB on GG refused); instruction-aware scan. Runs after the copier-header strip and the 1 MiB body refusal (scan sees at most 32 KiB). A bad checksum header neither outranks the scan nor argues for Korean.
- The scan is a linear sweep with a 256-entry Z80 length table (CB, ED, DD/FD, DD CB d op), at most 32,768 lookups, locals only, no allocation. Hit counts are per store site; Sega targets are `$FFFC` and above. A family F wins only when x_F >= T_F, x_F >= 2*s + 2, x_F >= 2*y + 2 over the runner-up, and its geometry fits (Zemina/Nemesis/Janggun multiples of 8 KiB; Zemina over 64 KiB; Nemesis exactly 128 KiB with signature; Korean `$A000` and `$3FFE` multiples of 16 KiB). The doubt zone refuses with the candidate named (a tie lands there); it never silently plays.
- Starting thresholds are conservative: T = 3 for every family until measured. Measurement on the owner's dumps is numbers only (command and exit status recorded, no bytes committed) and runs through `scripts/library.sh` if a private library is configured; if not, thresholds stay conservative and a seed records the measurement. Never blocks the phase.
- Hicom, Seo Jin and 188-in-1 are hash-only (no scan signature); documented.
- MAP-04 and MAP-05 evidence: no ROM bytes can be committed, so the runner cannot be driven with a real Seo Jin image. Split an internal `fact_to_info(fact, out)` in `src/cart.c` so `tests/cart/test_cartdb.c` runs the real Seo Jin, 188-in-1 and Janggun rows through it and asserts UNSUPPORTED_BOARD, board 0 and the reason id; the generated `tests/compat/catalog.tsv` carries the named-reason rows. The "ledger row" wording in MAP-04, MAP-05 and ROADMAP criterion 2 is amended to "catalog row" in this phase (Phase 12 owns the ledger); `ledger.csv` is not seeded. Add one assertion that every supported board has at least one table row so a slot-to-board regression fails.
- `--probe` and `cogmaster_cart_info` gain, appended after `title`: `board_source` (db=1, header=2, heuristic=3, default=4), `board_candidate`, scan counts, and a named-unsupported id (none, seo_jin, 188_in_1). A refusal returns UNSUPPORTED_BOARD with `board` 0, exit 77 in the runner, message naming board and reason.
- Fixtures are synthetic in-test buffers in `tests/cart/test_cart.c` (existing `fill`, `stores`, `put_header`, `seal` helpers; no manifest entries): sealed TMR SEGA decoy with Korean-looking stores stays Sega (source header); bad checksum plus few Sega stores refuses naming Korean; below threshold refuses naming the candidate; tie refuses; `3E 32 00 A0` instruction-boundary decoy stays Sega; synthetic Korean-style image selects Korean (source heuristic); 512-byte-prefixed image; body of exactly 1 MiB + 512. A DB geometry property test checks every row's size against its board's geometry table.

### Mapper arms (MAP-03, MAP-04, MAP-07)
- Existing `mapper.c` cases stay byte for byte; `default:` tail-calls `mapext_read` and `mapext_write` in new `src/mapper_ext.c`, so the Sega, Codemasters and ROM-only hot paths gain nothing. Board constants 4 and up, appended, never renumbered.
- One shared read for every ext board: `rom[(bank8[addr>>13] << 13) | (addr & 0x1FFF)]` with `bank8[i] < 2*pages` reduced at write time in uint32 (`% (2*pages)`); at most 128 banks fit a uint8. State adds `bank8[6]` and `raw[2]`, visited only for ext boards.
- Boards, per MAME BSD-3 `rom.cpp` and `sega8_slot.cpp` (facts cited in code comments with the source): Korean `$A000` (slots 0/1 fixed, a write to exactly `$A000` selects slot 2; `$FFFC-$FFFF` ignored); korean_nb gets its own board id using the Korean read path without the `$A000` register (exact to MAME for 32 KiB images; the ROM-only arm is not changed); Hicom (`$FFFF` selects a 32 KiB bank for `$0000-$7FFF`, `$8000-$BFFF` reads the first 16 KiB, every index reduced modulo image length, MAME's odd-page overread not copied); Zemina (writes to `$0000-$0003` select slots `$8000`, `$A000`, `$4000`, `$6000`; slots 0/1 fixed); Nemesis (Zemina with slot 0 powering up as the last 8 KiB bank and slot 1 as bank 1); 4PAK (`$3FFE`, `$7FFF`, `$BFFF`; slot 2 = ((reg0 & 0x30) + reg2) % pages; all-zero power-on). Documented register writes record no fault; any other write keeps the existing `rom_write` fault; over-range writes on a non-power-of-two image keep `mapper_range`.
- Janggun: `unsupported`, named reason "reverse bit 6 vs 7 unresolved (MAME and SMS Power disagree)". It becomes a board only if a later evidence run settles the bit; not built in this phase (MAP-04 allows unsupported with a reason). Seo Jin and 188-in-1: hash-only unsupported rows (MAP-05), exit 77.
- Fixtures: one parameterised WLA-DX `mapper_korean.asm` built into images for Korean, Zemina and 4PAK (the scan-detectable ones), 128 KiB with stamps at the start and end of each 8 KiB bank, judged through the mailbox (stamps at reset, per slot after writes, power-of-two wrap, `$FFFC-$FFFF` writes leave Korean and Zemina banks unchanged); Hicom, korean_nb and Nemesis are covered by C tests that force `m->cart.board` and call `mapper_reset`; both candidate behaviours cited where a register is ambiguous.
- Libretro coverage: one `tests/libretro/host.c` load of a Korean fixture (not `tests/retroarch`, which stays at 273 of 300). Guard `% (2*pages)` for `pages == 0`.
- Exhaustive tests `mapper.ext.*` under ASan: synthetic images allocated at exact size (32 KiB, 48 KiB, 128 KiB, 432 KiB, 800 KiB, 1 MiB), every value 0..255 to every documented register through the bus, `bank8[i] < 2*pages` after each write, a 20-line independent model, the low 13 address bits swept once per bank (read cost depends only on `addr>>13` and the low 13 bits) with every register value 0..255 on 128 KiB and 432 KiB, slot-boundary samples elsewhere, no fault for documented writes, a control write to `$1234` records `rom_write`.

### API, state, perf, proof (MAP-08)
- API 9 to 10 (all appended fields), ABI stays 1, behaviour revision 11 to 12, state format 11 to 12 (the `sizeof(cogmaster__mapper)` tripwire and `mapper_io` bounds change; ext fields visited conditionally). `abi.api_version` self-test still catches a header change without a bump; `tests/api/v10.c`; docs in `docs/embedding.md` (version table, names) and `docs/boards.md` (board prose; embedding.md gets two lines only).
- Perf gate (armed, +1.00 % vs merge base on boot_to_play, ROM-only): never add an `if` before the `cart.board` switch; ext boards live behind `default:`. Check `delta_bp` before review; fallback only: a `tests/perf/allow.txt` line (percentage, expiry within 60 days, reason naming Korean boards), deleted in Phase 7's first commit.
- MAP-08 proof: `git diff origin/main -- tests/audio tests/testcard_pins.cmake` is empty, and `git diff origin/main -- tests/built | grep -c '^-[^-]'` prints 0 (new built images legitimately ADD lines to `tests/built/expected.txt`; no existing line may change or go), plus the eight-lane `hash-equality` run URL on the head commit in the phase PR body; any removed or changed pin line is a stop. The new Korean, Zemina and 4PAK images are detected by the scan (source heuristic) or by owned `tests/cart/fixture_facts.txt` rows; say which in the plan.
- Budgets: check code 729 of 745 (limit rises 1 per 10 src lines): plan at most 25 counted check lines; keep the table in `.inc` (a `.h` table would inflate the src count); every new check gets a "Fault prevented:" line (rule 7); each plan at most 10 files (split larger); add a `PROVENANCE.md` line if any MAME text is copied.
- Title behaviour change: update `docs/runner.md`, `docs/boards.md` and `tests/api/v3.c` (`title[0] == 0` also on a hit); the 256-byte field stays.
- Plan/wave order: 1 generator slot table + compact rows + catalog.tsv + size.cartdb; 2 mapper split + state/API/tripwires; 3 boards (a) Korean/korean_nb/Hicom, (b) Zemina/Nemesis/4PAK in separate plans; 4 guess_board + board_source + decoys + probe/runner message; 5 unsupported rows, docs, gate evidence.

### Claude's Discretion
File and test names beyond those listed, exact wording, the Zemina/4PAK fixture image count, and the measured threshold values if a private library is available.
</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/cart.c` (`other_board`, loader, header checks), `src/cartdb.*` and `tools/cartdb.c`, `src/mapper.c`, `runner/probe.c`, `tests/cart/test_cart.c` helpers, `tests/cmake/expect_fail.cmake`, the WLA-DX mailbox fixture pattern, `scripts/compat-common.sh`.

### Established Patterns
- Every check ships a negative control with a named message; one parameterised source per fixture family; switch arms not function-pointer tables (`abi.globals`); hardware comments cite their source.

### Integration Points
- `include/cogmaster.h` (cart_info), `src/state.c`, `docs/boards.md`, `docs/embedding.md`, `quality.json`, `.github/workflows/ci.yml` "Size ceilings" step, `tests/compat/`.
</code_context>

<specifics>
## Specific Ideas

Owner preferences: copy-paste over dependencies; flat dependency tree; look for CI speed wins; automate verification.
</specifics>

<deferred>
## Deferred Ideas

- Janggun (until the reverse bit is settled), Seo Jin and 188-in-1 (hash-only unsupported).
- Real-dump threshold measurement if no private library is configured (seed).
- A pin-change awk that fails when a Sega-board row changes board (at pin-change time).
- The ROM-only arm's `$FF` read past the image end for 8 to 32 KiB images (not changed: MAP-08).
</deferred>
