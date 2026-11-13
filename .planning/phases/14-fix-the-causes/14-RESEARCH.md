# Phase 14: Fix the causes - Research

**Researched:** 2026-11-12
**Domain:** C emulator core fault recording, the Phase 13 diagnostic (C runner, sh/awk, CMake script tests), repository pin and docs gates. No external packages.
**Confidence:** HIGH for every code and test fact below (each was read in this session; discrete values are quoted). MEDIUM for hardware readings that the preparation documents only partly support (named under Premise checks). No dump was opened or looked for.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

Method: the owner's standing fan-out boilerplate was applied. Five advisor agents (fault policy and the boots check; the
109-row `mapper_range` group; the `unsupported_video_mode` groups; mapper families, `rom_write` and port faults; phase shape
and verification) each read the code and the preparation documents, applied the role lenses and ran a self-adversarial pass.
The orchestrator reconciled them (conflicts are named below), checked their citations against the tree and ran a
cross-cutting pass. No advisor saw a dump. Their cause claims are hypotheses until the counts in D-02 confirm them; the
decisions are written so that each one names the count that settles it. All areas were taken, no per-area vote was held
(the owner's boilerplate asks for one coherent set that is followed), and the earlier decision that the agent runs the
diagnostic in a shell where the owner passes `COGMASTER_ROM_DIR` stands.

- **D-01:** Plan order. Plan 1 (wave 1): reproduce the baseline, extend the diagnostic with the closed-vocabulary evidence
  words of D-02, create `14-FIX-OUTCOMES.md`, and write the per-fault-kind decision table (D-03) from hardware documents before
  any core edit. Then one plan per group that needs code, in descending rank, each ending in a `diagnose` re-run and a
  counts-only outcome row. Last plan: closures, the FIX-03 families, docs, and the revision paragraph. Waves are strictly
  sequential because every plan touches `src/` and the outcomes note, and each fix changes what the next group looks like (the
  record keeps only the first fault, so clearing one fault can expose the next one in the same cart). A fix is judged by whether
  its leaf shrank or moved, never by how many rows flipped. After the last group, run one final `diagnose`; a new leaf of 5 or
  more rows gets at most the single gap round the rules allow, otherwise it becomes a seed.
- **D-02:** Evidence first, one owner hand-off. Plan 1 adds a small set of closed-vocabulary words to the diagnostic line,
  each a bucket or class, never a value, address, byte or name, and each covered by the existing leak canary scan and the awk
  validator in `scripts/library.sh`: `faddr` class (`fffc`, `fffd-f`, other) and `pow2` (image page count a power of two, y or
  n) for `mapper_range`; `vdpmode` class (mode 4, graphics1, graphics2, text, multicolor, mixed, invalid) for
  `unsupported_video_mode`; a write-address class for `rom_write` (below `$100`, a foreign board's exact register, other below
  `$8000`, slot 2) and a port class for port faults (`$00-$06`, `$3E/$3F`, other below `$40`, `$40-$BF`) crossed with machine and
  region; `size` class and `header` (none, valid, bad); and `distinct1800` and first non-blank for the no-fault leaves. The
  owner exports `COGMASTER_ROM_DIR` once, at the start of the execute session. The baseline must print `run_fail=205` and
  `run_fail_not_in_ledger=0`. If the variable is absent the agent asks once (rule 6) and names the command; if the owner is
  not there, the agent still lands the documentation-grounded fixes and marks each outcome `fixed, unconfirmed on dumps`, and
  Phase 15's re-run is the confirmation batch. Never run `sh scripts/library.sh` with no argument, `run`, or the `library`
  workflow: they overwrite the tracked ledger and page. — **Reversibility:** reversible — the words are diagnostic output only.
- **D-03:** Each fault kind in the groups is decided from a hardware document, never from how many rows it would clear. The
  decision table lives in the outcomes note with one line per kind: genuine fault, benign by hardware, or emulation gap, plus
  the cited source (`.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` section 2.1, `SMS-SYSTEM.md` section 4 and C5,
  `SMS-VDP.md` section 7).
- **D-04:** A hardware-benign event stops being latched as the first fault, in the core, not in the runner. Applies to:
  `rom_write` on writes that hit no register (a ROM has no write line), `unmapped_write` on ports no device decodes,
  `unmapped_read` on the controller ports (open bus, already the documented principle in `docs/embedding.md`: things shipped
  games do are never faults), and `$FFFC` unused-bit writes (D-05). The boots predicate gets no per-kind filter: the core
  keeps only the first fault, so a benign first fault would hide a later real one and give a false pass. The kind constants
  stay (the enum is append-only and the header is terse); `docs/embedding.md` says those raise sites no longer fire.
  Reconciled advisor conflict: one advisor wanted `rom_write` kept for writes to another board's exact register as a
  wrong-board canary, another wanted it dropped outright. Decision: drop it. The diagnostic's mapper, `board_source` and
  liveness facts are the misdetection signal. Evidence trigger: if the `rom_write` address class shows a whole board family's
  exact-register writes (Codemasters, Korean, Zemina, 4 PAK registers), fix that family's board choice with a table row or a
  header rule, never by loosening the scan thresholds. — **Reversibility:** costly — a documented public fault surface and the
  saved-state fault record change with revision 18.
- **D-05:** `mapper_range` is two raise sites with different answers. Split by `faddr` and `pow2` before editing. A `$FFFC` write
  with bits in the unused mask (`SEGA_UNUSED` in `src/mapper.c`) is inert on retail hardware (SMS Power mappers table; no
  known software uses them), so the raise and the define are removed and `p->ctrl = v` stays. The bank-past-the-end fault on a
  non-power-of-two image stays (DEC.37: banks wrap modulo pages; real chips are power-of-two so the odd size is the artefact,
  and the content at that bank is unknown). Advisor inference, to be confirmed by the counts: about 100 of the 109 rows are
  `$FFFC` writes from early init (first-fault PCs are early-init addresses shared by several carts, so library or SDK code). If
  instead most rows are `fffd-f` on a non-power-of-two image, this decision is wrong and the open-bus question for odd image
  sizes takes over; stop and re-plan that group with the evidence. The one `fault_no_display.mapper_range` row inherits this
  group's outcome.
- **D-06:** The Sega bank shift (`$FFFC` bits 1 to 0 adding `$18/$10/$08`) is applied today to every image size, so junk writes
  could displace banks on small images. Do not change the shift in the first pass. Take one `--trace` run on one exemplar of
  the largest `fffc` bucket to see the value written. Only if it shows bits 1 to 0 set in the common case, add the gate
  (apply the shift only where address lines are wired, images of 32 pages or more) with a fixture; otherwise leave it and say
  why in the outcomes note. Any pin that gate moves is named. — **Reversibility:** reversible.
- **D-07:** The 36-row `fault_static.unsupported_video_mode` group is closed with a stated reason, not fixed: legacy TMS9918
  modes (Graphics I and II) have no renderer by design (DEC.03, SEED-006, and REQUIREMENTS "SG-1000" out of scope). Reconciled
  advisor conflict: one advisor recommended a new `src/vdp_tms.c` of about 250 lines; another recommended closing. Decision:
  close, because SMS-VDP section 7 is only a summary with the datasheet unavailable as text, a renderer is a feature phase
  (SEED-006 sized it as one), and the roadmap says no cause is assumed and fixes follow the counts. This is the largest judgement
  call of the phase and the owner can promote it: the closure records the `vdpmode` counts as the trigger evidence in a
  breadcrumb on SEED-006 ("the library shows N rows needing legacy modes"). Evidence gate: if any of the 36 shows mode 4 (M4=1),
  that is a decode bug, not a missing mode, and it is fixed in this phase. The closed reason also makes a refusal distinguishable
  from a bug: the mode nibble goes in the fault record's `address` field (passed as 0 today; no ABI change, documented in
  `docs/embedding.md`), and the diagnostic prints the `vdpmode` class. — **Reversibility:** reversible.
- **D-08:** The 6-row `fault_alive.unsupported_video_mode` group (5 at the same PC) is decided by evidence. If the counts show a
  supported mode-4 frame displayed before the fault and the fault after the first non-blank picture (a restart transient: the
  display latch is still on while the guest rewrites the mode registers), fix it in the core: raise the fault only where the
  display latch is sampled at the line latch, so a transient that recovers within a line is not a fault; the pixel path keeps
  drawing backdrop. This adds no saved state and no new fault kind. Repoint the existing unsupported-mode assertions
  (`tests/vdp/test_background.c`, the light-phaser timing test) at combinations that still fault. If the counts do not show that
  shape, close the group with the D-07 reason.
- **D-09:** The boots check is sound; keep `boots@600` and `distinct >= 2`. Stated reason: the passing rows' first-change
  distribution ends at 361-420 with nothing between 421 and 600, a gap wider than any before it, so no passing cart is clipped;
  the only failing leaves with no fault are two rows (`dark_no_fault`, `static_no_fault`), too few to show a distribution gap,
  so any new frame would be chosen by rows flipped, which the note forbids; and `boots@600` is part of the ledger
  configuration, so moving it migrates every row (the planner verifies where it is keyed). Settle the two rows from
  `distinct1800` and first non-blank: if one shows `distinct1800 >= 2` it is a lone late-first-picture row, closed with that
  reason, not a reason to move the cap; if it shows 1, it is a static picture and the existing `static-display` fixture is the
  intended reject. `dark_no_fault` (never draws) keeps failing whatever its size, and the never-draws and static-display seeds
  stay. If the cap ever does move, the fixture is a synthetic cart blank until a frame between 600 and the new cap and animated
  after, which passes at the new cap and fails at 600, next to the unchanged never-draws seed.
- **D-10:** Zemina (8, all static), `korean_nb` (5, all static) and `korean` (2 static, 2 alive) each get a stated cause from the
  cross-cut crossed with the new `vdpmode` and `size` words. Starting hypothesis, not a decision: the mapper arithmetic and
  power-on banks match the documented table and have tests (`mapper.ext.zemina`, `mapper.ext.korean_nb`), so these rows are the
  legacy-mode cause of D-07 (they are headerless images of 48 KiB or less and port-style titles), and the 21 `none` static
  rows with them, with no mapper or scan change. The arithmetic shows at least one family or `none` row is a `rom_write` static
  row (40 static rows = 36 unsupported mode + 4 `rom_write`), so read the cross-cut before closing a family. If a family row
  turns out to be mode 4, that is a real fast-boot or mapper bug: fix it with a fixture. The two `korean` alive rows go with
  the alive groups they fall in.
- **D-11:** Ports: both groups are fixed in the core by D-04, each justified from `SMS-SYSTEM.md` (a write to an undecoded port
  is ignored; reads of `$00-$3F` return open bus). Evidence trigger: if the port class shows mirror writes on a Japanese profile
  under full decode, the right fix is a profile or decode decision (the `input_mismatch` group), not a benign list; stop and
  re-plan that leaf.
- **D-12:** The behaviour revision goes from 17 to 18 once, in the first behaviour-changing plan, and later plans extend its
  paragraph. It touches `include/cogmaster.h` (the revision define), the exact pin in `tests/core/test_core.c`, the state header
  in `src/state.c` (saved states from older revisions are refused, so state fixtures move), a `docs/embedding.md` sentence and
  the ratchet's dated revision exceptions. Frame and audio pins must not move: the fixes change what is recorded, not what
  executes (except D-06 and D-08 if their gates fire, which name their pins). Cheapest proof with no new test code: build
  `origin/main` in a worktree, write its `hashes.txt` with the existing hashes test, and compare it to the head file with
  `tests/cmake/hashes.cmake` in compare mode (the planner confirms the exact command); any difference is a moved pin and is named
  in the outcomes note. On the dumps, `run_fail_not_in_ledger=0` after every fix is the set-level proof that no passing row now
  fails; Phase 15's ratchet is the authoritative gate. — **Reversibility:** costly — the state format refuses old saves.
- **D-13:** Every core fix has a CTest case on the `ci` workflow with a hand-assembled synthetic fixture written from the
  hardware document (no cart-derived bytes or opcode sequences, listed in `tests/roms/manifest.txt`) with a positive and a negative
  control: a write to an exact mapper register still behaves, a bank past the end of an odd image still faults, a write to a
  non-register ROM address no longer faults, a port access no longer faults, a mode that is still unsupported still faults. Edit
  the tests that assert the dropped kinds rather than adding new files: `tests/bus/test_bus.c` (`test_fault_mapper_range`),
  `tests/mapper/test_mapper.c` and `test_mapper_ext.c`, `tests/io/test_io.c`, `tests/core/test_state.c`, `tests/diag/expected.csv`
  and the `in-a-00` seed use in `tests/runner/expect.cmake` (the planner greps for the rest). No repository-check code is added and
  no fix is keyed on a catalog id, hash or title.
- **D-14:** `14-FIX-OUTCOMES.md` is the record, counts only, with a header like the Phase 13 note (core commit, behaviour
  revision, UTC date, command, exit status) and a table `rank | leaf | rows (13 note) | outcome | evidence | residual leaf
  counts`, citing the Phase 13 note by rank and leaf name as printed. A closed-with-reason outcome has four parts: the class
  (benign by hardware, not emulated by design, no evidence), the cited source (never a row count), why the row staying `fail` is
  honest, and what evidence would reopen it. Groups under 5 rows are reported as printed; `dark_no_fault` keeps failing;
  `static_no_fault` gets one run with a pad press and is then closed with its reason (the Phase 13 deferral).
- **D-15:** One pull request from `phase/14-fix-the-causes`, titled as a Conventional Commit `fix(core): ...` (a patch release;
  `feat(runner)` only if the diagnostic gains a documented flag, which it should not), one atomic commit per task, no
  BREAKING CHANGE text. The ledger, the page, `ci.yml` and every timeout are untouched; the page wording changes only if the
  boots definition changes, which D-09 says it does not.

### Claude's Discretion

- The exact names and vocabulary of the evidence words in D-02, and whether they ride on the existing diag line or sub-rows.
- Whether the `address` field carries the mode nibble (D-07) or the runner reads the VDP registers another way, as long as no
  public header field is added silently (stop and report if one is needed).
- The number of plans beyond the order in D-01, and which exemplar the `--trace` run uses (never named in any file).

### Deferred Ideas (OUT OF SCOPE)

- A TMS9918 Graphics I and II renderer and SG-1000 mode: SEED-006, a phase of its own; the `vdpmode` counts from D-07 are its trigger evidence.
- Gating the Sega bank shift by image size, unless D-06's trace shows it is needed.
- A public counter or fault flag for benign events: rejected for now (header growth for one consumer); the diagnostic cross-cuts carry the signal.
- A Start-press rerun campaign beyond the one `static_no_fault` row.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FIX-01 | Each cause group of 5+ rows is fixed in the core or the boots check, or closed with a stated reason, in descending order of size | Raise-site inventory (section 1), outcome-row format (section 9), plan order (section 11) |
| FIX-02 | If the boots check rejects carts that run correctly it is corrected, with a late-picture fixture; else recorded sound | `boots@600` keying and `cogmaster__expect_boots` (section 5); existing never-draws and static-display seeds (section 5) |
| FIX-03 | zemina, korean_nb, korean rows each have a stated cause, fixed or refused with a closed reason | Words needed from the diagnostic (section 3); cross-cut arithmetic (Premise checks); legacy-mode analysis (section 7) |
| FIX-04 | Every core fix has a ci-workflow test; any moved frame/audio pin is named, justified, recorded with a revision bump | Test edit map (section 10), pin-proof command (section 4), revision touch list (section 2) |
</phase_requirements>

## Summary

The fault record has no consumer inside the core: `cogmaster__fault_record` only stores the first fault and nothing in `src/` reads `fault_present` except save/load and reset [VERIFIED: grep of `fault_present` in src, runner, libretro, host: only `src/instance.c`, `src/fault.c`, `src/state.c`, `src/internal.h`]. The only behavioural consumer is the runner's boots predicate, `e->display && e->distinct >= 2u && !e->fault` [VERIFIED: runner/expect.c:181-184]. So D-04, D-05 and D-07 cannot move a frame or audio hash, which is what makes the base-versus-head proof cheap. The only changes that can move a pin are D-06's shift gate and D-08's raise relocation, and D-08 moves the fault's frame, line, dot and pc, not any pixel.

There are 13 raise sites to touch for D-04/D-05 (4 in `src/mapper.c`, 7 in `src/mapper_ext.c`, 2 in `src/io.c`) plus one in `src/vdp_render.c` for the nibble; the rest of `bus.c` and `io.c` (memory-side `unmapped_read` and `unmapped_write`) stay. About 15 existing test sites assert a dropped kind; the work is flipping those from `expect_fault` to "no fault", while keeping a positive control per kind that still fires (`ed_undefined`, memory-side `unmapped_read`/`unmapped_write`, bank past the end of a non-power-of-two image, an unsupported mode).

Four premise problems need the planner's attention before plan 1 is written (see Premise checks): the `vdpmode` word needs a core edit, so it contradicts "before any core edit" in D-01; `distinct1800` and first non-blank already exist on the diagnose line; two files named in D-13 (`tests/core/test_state.c`, `tests/diag/expected.csv`) assert no dropped kind; and `SEGA_UNUSED` includes the shift bits 1-0, which interacts with D-06.

**Primary recommendation:** Plan 1 lands the diagnostic words (runner + awk + tests) together with the one tiny core edit that puts the mode nibble in `addr` and the revision bump 17 to 18; it captures the baseline counts into `14-FIX-OUTCOMES.md` before any benign kind is dropped. Then fix in rank order: `$FFFC` raise (rank 1), `rom_write` (ranks 3 and 7), ports (ranks 4 and 5), with the repointed seeds and tests in the same plan that drops each kind.

## Premise checks

Read these first. Each one is a place where the code or documents differ from what a decision assumes. None reopens a decision; each says what the plan must do about it.

1. **`vdpmode` needs a core edit, so "before any core edit" (D-01) cannot hold for it.** The runner can read no VDP register through the public API: `cogmaster_peek` takes `COGMASTER_SPACE_BUS` only ("`cogmaster_peek(const cogmaster *m, uint32_t space, uint32_t addr); /* above 255: none */`", include/cogmaster.h; `tests/api/v2.c` asserts `cogmaster_peek(m, COGMASTER_SPACE_BUS + 1u, 0x0002u) > 255u`). The nibble therefore must ride in `addr` (D-07's own choice), which is a one-line change at `src/vdp_render.c:189` (`cogmaster__bus_fault(m, 0u, ...)` becomes `cogmaster__bus_fault(m, nibble, ...)`) and changes the saved fault record for kind 6. Recommendation: plan 1 carries that edit, its test, and the 17 to 18 bump (it is then "the first behaviour-changing plan" D-12 refers to). The baseline `run_fail=205` is unaffected because no fault kind or presence changes. [VERIFIED: src/vdp_render.c:187-191, runner can only reach `cogmaster_peek`/`cogmaster_get_*` public calls]
2. **`distinct1800` and first non-blank are already printed.** The diagnose line already carries `first_nonblank=`, `distinct600=`, `distinct1800=`, `first_change=`, `ram=` [VERIFIED: runner/diag.c:125-135, quoted in section 3]. Only `faddr`, `pow2`, `vdpmode`, the write class, the port class, `size` and `header` are new.
3. **"mode 4" can never be a faulting `vdpmode`.** `vdp_supported` accepts nibbles 8, 10 and 12 on every chip, so a fault with M4=1 means one of: nibble 9 or 13 (invalid text on every chip), or nibble 11, 14 or 15 on the SMS1 profile (invalid text there). The nibble is `M4 M3 M2 M1` (SMS-VDP 2.5). Define the classes as: `graphics1` (0), `text` (1), `graphics2` (2), `multicolor` (4), `mixed` (3, 5, 6, 7), `invalid` (any nibble with M4=1), and keep `mode4` as a defensive class that should print zero rows (nibble 8, 10 or 12 recorded would prove `vdp_supported` and the record disagree). Cross `vdpmode` with machine (`$3` of the record: `sms`, `sms1`, `gg`) in the sub-row, so an `invalid` row on `sms1` (the catalog's SMS1 label) is told apart from a true invalid combination. D-07's evidence gate is then: any `invalid` row is read as "M4=1, decode question", `mode4` > 0 is a decode bug. [VERIFIED: src/vdp_render.c:54-63; SMS-VDP.md:75]
4. **`SEGA_UNUSED` is `0x93`: it includes bits 1-0, the bank-shift bits.** The mask reads `#define SEGA_UNUSED 0x93u` with the comment "$FFFC b7 (ROM write enable), b4 (RAM over work RAM) and the bank shift b1-0: no shipped game uses them (section 2.1)" [VERIFIED: src/mapper.c:24-26]. D-05 removes the raise and keeps `p->ctrl = v`, so the shift stays applied (D-06) with no fault. That is correct, but it means a `$FFFC` write of 1, 2 or 3 is today a fault *and* a displaced bank, so the D-06 trace matters: a common `$FFFC` value with bits 1-0 set would be both. Also note (src/mapper.c:111-120) the raise fires before `p->ctrl = v`, and `bank_write` adds `shift[p->ctrl & SEGA_SHIFT]`.
5. **SMS-SYSTEM section 4 does not say a write to an undecoded port is ignored.** It documents read behaviour ("$00-$3F even ($3E) | SMS1 open bus; SMS2/GG $FF | memory control") and that on full-decode machines "the $3E/$3F mirrors across $00-$3F do not exist". It is silent on what an OUT to such a port does. D-11's "a write to an undecoded port is ignored" is an inference (a Z80 OUT to a port with no device has no effect) and should be tagged as such in the decision table: cite C5/section 4 for the read half, and state the write half as the absence of any documented fault. [VERIFIED: .planning/preparation/SMS-SYSTEM.md:133-168] The same section also says GG-mode ports `$00-$06` "in SMS mode ... become $3E/$3F mirrors and read $FF", while the core treats them as undecoded (`tests/io/test_io.c` "Full decode: a mirror of $3E latches nothing"). That is an existing document/code difference this phase does not need to resolve; D-04 makes it silent rather than a fault.
6. **SMS-CARTRIDGES 2.4 does not call an odd-size over-range bank a fault.** It says "Power-of-two ROMs mirror, so over-range bank numbers are harmless ... MAME reduces `value % page_count` ... **Recommendation:** round the image up to a 16 KB multiple with $FF, then `bank % pages`". The fault at `src/mapper.c:33-38` (comment cites section 2.4) is the core's own marker. D-05 keeps it; the outcome note should say "kept as a marker, the bank still wraps" and not cite section 2.4 as the source of a fault. [VERIFIED: SMS-CARTRIDGES-AND-PERIPHERALS.md:115-117; src/mapper.c:33-46]
7. **SMS-VDP section 7 supports far fewer legacy-mode titles than 36.** It names one SMS title in a TMS mode ("F-16 Fighting Falcon (SMS) runs in a TMS mode") and says "SG-1000 cards and the Mark III's SG-1000 compatibility are the main consumers". The 36-row group is therefore a hypothesis to test with the `vdpmode` counts, not a documented fact. Note the arithmetic in the cross-cut: korean 2 + korean_nb 5 + none 21 + zemina 8 = 36 fault_static rows on those mappers, equal to the 36 `unsupported_video_mode` rows, but the cross-cut also holds sega 3 and nemesis 1 static rows, so the identity is suggestive, not proof (D-10 already says to read it). That Korean/Zemina carts are ports that use a TMS mode is [ASSUMED]. SEED-006's own "When to Surface" says it "is small enough to ride inside v2.0 if the VDP's legacy modes are already needed for an SMS title in the ledger", which is exactly the evidence D-07's breadcrumb records. [VERIFIED: SMS-VDP.md:187-192; .planning/seeds/SEED-006-sg1000-mode.md; 13-CAUSE-GROUPS.md:34-51]
8. **D-12's "state fixtures" and "dated revision exceptions" are conditional or empty.** There is no stored saved-state byte fixture in the tree (grep over `tests/` for `format_version`/`state_magic`/`STATE_FORMAT` finds only api-version and field-name tests; none holds a golden state), and `src/state.c:109` writes `COGMASTER_BEHAVIOUR_REVISION` through the macro, so no edit to `src/state.c` is needed (its comment at line 34 names "behaviour revision 16" as a historical header-offset note and is already stale). `tests/compat/allow.txt` holds two comment lines only; an exception is needed only if an existing ledger `pass` row would change, which none of the fixes can cause [VERIFIED: tests/compat/allow.txt; scripts/compat-common.sh:93-103; src/state.c:109,34]. Touch list in section 2.
9. **D-13's file list is partly wrong.** `tests/core/test_state.c` feeds `cogmaster__fault_record` its own `sample_fault()` values (kinds 5+seed) and asserts no dropped kind. `tests/diag/expected.csv` has no fault expectation (it judges mailbox values; a grep for the kind names finds nothing). The files that do need edits are in section 10, and include two D-13 did not name: `tests/mapper/test_eeprom.c` (lines 702, 707) and `tests/api/v2.c` (lines 138-141), plus the seed uses in `tests/runner/tests.cmake` and `tests/library/diagnose.cmake`.
10. **`tests/roms/manifest.txt` does not list hand-assembled test bytes.** It lists committed non-text files and third-party data, and carries comment lines for built-only images (`# Owned ... images are built only ...`). Inline byte arrays in a C test (as `tests/bus/test_bus.c` does) and seeds generated by `tests/fuzz/seeds.c` need no manifest entry; only a new `.asm` fixture would get a comment line [VERIFIED: tests/roms/manifest.txt:1-24; scripts/hygiene.sh:9-42]. Recommendation: no new `.asm`.
11. **The runner and the diagnose scripts are not "repository-check" lines.** `scripts/library.sh` and `scripts/library.cmake` are classified `operation`, and only kind `check` counts toward the tenth [VERIFIED: tests/policy/repository-checks.txt:53-54; tests/cmake/budgets.cmake:46-52]. Current margin: 887 check lines against a limit under 940 (`counted*10 < core`, core = 9,403 src lines at research time). Deleting `src/` lines lowers the limit by 1 per 10 lines deleted; adding no `tests/cmake/*`, `scripts/*.sh` of kind check or `tests/compat/*.cmake` lines keeps the gate unchanged.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Deciding whether an event is a fault | Core recorder (`src/mapper.c`, `src/mapper_ext.c`, `src/io.c`, `src/vdp_render.c`) | none | The record keeps only the first fault, so policy must be at the raise site (D-04) |
| Mode nibble in the record | Core (`src/vdp_render.c`) | Runner reads `fault.addr` | Public API exposes no VDP register read |
| Boots verdict | Runner (`runner/expect.c`) | Library judge (`scripts/library.cmake`) | Unchanged; sound per D-09 |
| Evidence words (classes) | Runner (`runner/diag_class.h`, `runner/diag.c`) | `scripts/library.sh` validator and reducer | Pure functions of record fields and `cogmaster_probe`; validated at the shell boundary |
| Pin proof | CI test `runner.hashes_file` + `tests/cmake/hashes.cmake` compare | owner dumps `run_fail_not_in_ledger=0` | No new test code |
| Outcome record | Planning note `14-FIX-OUTCOMES.md` | SEED breadcrumbs | Counts only |

## Standard Stack

No external package is added or changed. The phase edits C in `src/` and `runner/`, POSIX sh/awk in `scripts/library.sh`, and CMake script tests; `cmake` 4.4.3 and `ctest` are on PATH [VERIFIED: `cmake --version`]. Per the project rule, "Another copy and paste beats another dependency".

### Package Legitimacy Audit

Not applicable: no package is installed. **Packages removed due to [SLOP] verdict:** none. **Packages flagged [SUS]:** none.

## 1. Raise sites and what each does

All verified by reading the files this session. `cogmaster__bus_fault(m, addr, device, kind)` calls `fault_at(m, m->bus.op_pc, addr, m->bus.op, device, kind)` which fills `frame`, `line`, `dot = bus.t >> 1`, `pc = op_pc`, `addr`, `opcode = op`, `width = 8` and records only if none is present [VERIFIED: src/bus.c:318-340; src/fault.c:7-14].

| Kind to drop or change | Site (file:line) | What it does now | Action |
|---|---|---|---|
| `mapper_range`, `$FFFC` unused bits | src/mapper.c:111-114: `if ((v & SEGA_UNUSED) != 0u) { cogmaster__bus_fault(m, addr, COGMASTER__DEVICE_MAPPER, COGMASTER__FAULT_MAPPER_RANGE); }` then `p->ctrl = v;` | Faults when b7, b4 or b1-0 of an `$FFFC` write is set, then stores `v` | Delete the `if`; delete `#define SEGA_UNUSED 0x93u` (src/mapper.c:26) and its comment (24-26). Keep `p->ctrl = v`. |
| `mapper_range`, bank past end | src/mapper.c:39-46 `bank_write`: `if (v >= pages && (pages & (pages - 1u)) != 0u) fault`; src/mapper_ext.c:30-36 `ext_check`: `highest >= 2u * pages && (pages & (pages - 1u)) != 0u` | Flags an over-range bank on a non-power-of-two image; the bank still wraps (`bank_of`, `ext_bank`) | **Keep both** (D-05) |
| `rom_write`, Sega | src/mapper.c:121-127: the final `else` of the `addr >= 0xFFFCu` / `addr >= 0x8000u && RAM_ON` chain (line 126) | Any write below `$8000`, or at `$8000-$BFFF` with cart RAM off | Delete the `else` |
| `rom_write`, Codemasters | src/mapper.c:137-141: `else if (addr < 0xC000u)` (line 140) | A write below `$C000` that is neither a register (`$0000`/`$4000`/`$8000` exactly) nor RAM | Delete the branch |
| `rom_write`, Korean | src/mapper_ext.c:101-103 (fault at 102) | Any write below `$C000` except `$A000` | Delete the `else if` |
| `rom_write`, Hi-Com | src/mapper_ext.c:115-117 (116) | Any write below `$C000` except `$FFFF` | Delete |
| `rom_write`, Zemina/Nemesis | src/mapper_ext.c:130-132 (131) | Below `$C000`, except `$0000-$0003` | Delete |
| `rom_write`, 4 PAK | src/mapper_ext.c:159-161 (160) | Below `$C000` except `$3FFE`, `$7FFF`, `$BFFF` | Delete |
| `rom_write`, Sega EEPROM | src/mapper_ext.c:184-186 (185) | Below `$C000` except the open window | Delete |
| `rom_write`, default arm (Korean no-bank and the ROM-only board, which reaches `mapext_write` through the `default:` of `cogmaster__mapper_write`) | src/mapper_ext.c:192-195 (193) | Any write below `$C000` | The arm becomes empty; keep its comment ("The no-bank Korean board has no register ... Writes to ROM change nothing") |
| `unmapped_read`, ports | src/io.c:461-464: `if (p < 0x40u) { /* $3E and $3F are write-only; nothing drives a read there. */ cogmaster__bus_fault(m, port, COGMASTER__DEVICE_CTRL, COGMASTER__FAULT_UNMAPPED_READ); }` then `return cogmaster__io_open_bus(m);` | Faults on a read of `$00-$3F` (the GG `$00-$06` ports return earlier at 451-460) | Delete the `if`. **Keep** the `return cogmaster__io_open_bus(m)` so the counter still counts and the value is unchanged. |
| `unmapped_write`, ports | src/io.c:507-509: `if (p < 0xC0u) { cogmaster__bus_fault(m, port, COGMASTER__DEVICE_NONE, COGMASTER__FAULT_UNMAPPED_WRITE); }` | A write nothing latched below `$C0` (reached on full-decode consoles for `$00-$3D`, and for `$80-$BD` that the VDP does not answer) | Delete the `if`. Update the function's header comment (src/io.c:469-472 "on a full-decode console a write nothing else latches is unmapped_write"). |
| `unsupported_video_mode` | src/vdp_render.c:187-191: `if (at < end && m->vdp.display && !vdp_supported(m)) { cogmaster__bus_fault(m, 0u, COGMASTER_FAULT_DEVICE_VDP, COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE); }` | Raised from `render_until` whenever pixels are pending, the display latch is on and the combination is unsupported; `render_until` is called from `cogmaster__vdp_write` (src/vdp.c:398), a status read (src/vdp.c:297), the line event at tick 512 (src/vdp.c:186) and the Light Phaser sensor (src/gun.c:140) | Plan 1: pass the nibble as `addr`. D-08 only if counts show the transient shape. |

**Stay as they are** (memory-side, documented in `docs/embedding.md` rows 1 and 2): src/bus.c:186 (`unmapped_read`, cart or RAM off the bus), src/bus.c:255 and 260 (`unmapped_write` RAM off / cart off), src/io.c:197 and 210 (RAM-off writes at `$FFF8-$FFFF`), and src/bus.c:353 (`ed_undefined`).

**Nibble helper.** `vdp_supported` computes `((uint32_t)(r[0] & 4u) << 1) | ((r[1] & 8u) >> 1) | (r[0] & 2u) | ((r[1] & 16u) >> 4)` which is `M4 M3 M2 M1` [VERIFIED: src/vdp_render.c:54-63]. Factor that expression into a `static uint32_t vdp_mode(const cogmaster *m)` used by both `vdp_supported` and the raise.

**D-08 shape, if it fires.** The latch is at `src/vdp.c:190-193`: `if (t == VDP_LATCH_TICK) { m->vdp.hscroll = m->vdp.reg[8]; m->vdp.display = (uint8_t)((m->vdp.reg[1] >> 6) & 1u);`. The raise would move to just after the display assignment, guarded by `cogmaster__vdp_render_blocked(m)` (src/vdp_render.c:68-71 already returns `display != 0 && !vdp_supported`), and the `render_until` raise would be deleted. Consequences the plan must carry: (a) frame/line/dot/pc of every unsupported-mode fault change (the record then names the instruction running at tick 600, not the VDP write); (b) `src/gun.c:137` keeps calling `cogmaster__vdp_render_blocked` and stays correct (it only declines to render ahead); its comment and the sentences at `docs/hardware/video.md:55-57` and `docs/controllers.md:178-181` ("the renderer's own schedule records the first fault there") must be reworded; (c) `tests/ports/test_gun.c:907-920` `test_fault_neutral` pins `p->fault.line == 0` and `p->fault.dot == COGMASTER__FIELD_W` ("the end of the first line's active period") and must be repointed to the latch tick; its `memcmp(&g->fault, &p->fault, ...)` neutrality assertion must still hold. `tests/vdp/test_background.c:387-410` (`unsupported()`) only asserts presence, device and kind, so it survives as is.

## 2. The fault record, the nibble and the revision

- **Record:** `struct cogmaster__fault { uint64_t frame; uint32_t line, dot; uint16_t pc, addr; uint8_t opcode, width, device, kind; }` [VERIFIED: src/internal.h:50-54]. Public `cogmaster_fault` has `uint16_t pc, addr;` [VERIFIED: include/cogmaster.h:225-232]. `addr` is a free `u16`: `fault_io` reads `cogmaster__sio_u16(io, "fault_addr", &f->addr, 0u, UINT16_MAX)` [VERIFIED: src/state.c:175], so a nibble (0-15) in `addr` needs no ABI or state-format change and passes the load validation. `cogmaster__fault_record` keeps only the first (`if (m->fault_present) return;`).
- **Public header:** `#define COGMASTER_BEHAVIOUR_REVISION 17` at include/cogmaster.h:16; `COGMASTER_STATE_FORMAT_VERSION 16` at :17 (leave); `COGMASTER_API_VERSION 14` (leave). No new field or constant. The header stays terse; behaviour prose goes to `docs/embedding.md` (the header is 9,951 bytes against a 24,576 budget).
- **Revision 17 to 18 touch list (complete):**
  1. `include/cogmaster.h:16`.
  2. `tests/core/test_core.c:435-436`: the comment "Revision 17 is the Graphic Board refusal; the api pins say 'at least', so a revert fails." and `CHECK_EQ_U64(v.behaviour, 17u);` become 18 with a one-clause comment of what revision 18 is (the fault record stops recording benign events; no planning labels).
  3. `docs/embedding.md`: one sentence in the Faults section (it has no revision-17 sentence today; the API-history lines 55-59 each carry "behaviour revision N"). Embedding.md is 36,442 bytes against a 38,912 ceiling.
  4. `src/state.c`: no edit (macro at line 109). Optional: refresh the stale comment at line 34.
  5. Other `COGMASTER_BEHAVIOUR_REVISION` readers are `>=` pins (`tests/api/v9.c`..`v14.c`, `v1.c:30`) or the macro itself (`tests/api/v1.c:61`, `scripts/library.sh:52,236`, `.github/workflows/ci.yml` awk reads): untouched.
  6. `tests/compat/allow.txt`: untouched unless a named existing `pass` row moves (none expected). If one did, the line format is `YYYY-MM-DD <id> <old-predicate> <reason words> behaviour-rev=<new revision> expires=YYYY-MM-DD`, the revision must exceed the base and equal the head [VERIFIED: tests/compat/allow.txt:1; docs/conformance.md:45-50].
- **Old saved states are refused** by `fixed(io, "behaviour_revision", COGMASTER_BEHAVIOUR_REVISION, 4u)` [VERIFIED: src/state.c:109]; same-build round trips (libretro serialize, `tests/core`) are unaffected.
- **`tests/testcard_pins.cmake`** says "A changed card changes these only together with the behaviour revision" and holds the SMS and GG frame-60 test-card SHA-256 and the PNG SHA-256; the test card runs no CPU and no fault, so these cannot move [VERIFIED: tests/testcard_pins.cmake:1-9; docs/embedding.md:369-370].

## 3. The diagnostic: how the line is built and where words go

**Runner side.** `runner/diag.c:cogmaster__diag_print` prints one `printf`:
`diagnose 1 verdict600=%s first_display=%s first_nonblank=%s first_change=%s distinct600=%lu distinct1800=%lu ram=%s fault=%s dev=%s pc=%s pcslot=%s where=%s fault_frame=%s order=%s mapper=%s source=%s\n` [VERIFIED: runner/diag.c:125-135]. It already holds `fault` (from `cogmaster_get_fault`), `cart` (from `cogmaster_probe(image, image_size, &cart)`: `image_size`, `body_offset`, `header_at`, `checksum_ok`, `board`, `board_source`) and `image_size`. Pure class functions live in `runner/diag_class.h` (`cogmaster__diag_bucket`, `_order`, `_pcslot`, `_where`) and are unit-tested by `runner.diagnose.class` (`cogmaster-test-bench diag_class`, `tests/runner/test_runner.c:227-`, registered at `tests/runner/tests.cmake:1944-1947`). New classes belong beside them so the new words get chosen-value tests with no emulation:
- `faddr`: `fault.addr == 0xFFFC` -> `fffc`; `0xFFFD..0xFFFF` -> `fffd-f`; else `other` (only meaningful when `kind == mapper_range`; print `none` otherwise).
- `pow2`: pages = ceil((`image_size` - `body_offset`) / 16384); `y` when a power of two, else `n`. `cart.c:387` shows the core's `pages = size / CART_PAGE` after padding, so the runner's rounding must agree [VERIFIED: src/cart.c:339-349,387].
- `vdpmode`: from `fault.addr & 15` when `kind == unsupported_video_mode` (classes in Premise check 3).
- write class for `rom_write`: `<$100`, `foreign` (exact register of a board other than `cart.board`: Codemasters `$0000/$4000/$8000`, Korean `$A000`, Zemina/Nemesis `$0000-$0003`, 4 PAK `$3FFE/$7FFF/$BFFF`), `other<8000`, `slot2`. Evaluate "foreign" before `<$100` or the Zemina/Codemasters `$0000-$0003` registers hide inside the first bucket.
- port class (port = `fault.addr & 0xFF`): `00-06`, `3e-3f`, `other<40`, `40-bf`; cross with machine and region from the record (`$3`, `$4`).
- `size` (class by `image_size - body_offset`, e.g. `le16k`, `le32k`, `le48k`, `le64k`, `le128k`, `le256k`, `le512k`, `gt512k`, `odd`) and `header` (`none` when `header_at == 0`, `valid` when `checksum_ok`, else `bad`).

**Closed-vocabulary hazards (leak canary).** `tests/library/diagnose.cmake:diag_scan` fires `leak_digest` on any run of 8 characters from `[0-9a-fA-F]`, `leak_path` on `[/\\]`, `leak_name` on `\.(sms|gg)`, `leak_bytes` on any byte outside `0x20-0x7E` and LF [VERIFIED: tests/library/diagnose.cmake:40-43]. No class word may contain a slash, backslash, `.sms`/`.gg`, or 8 consecutive hex characters (`fffd-f`, `3e-3f`, `le128k` are fine; a word such as `deadbeef` or a compound with the pc is not).

**Shell side.** `scripts/library.cmake` runs `cogmaster-run --diagnose ... FILE` and appends `rec ${judged} ${machine} ${region} ntsc ${fm} ${board} ${line}` [VERIFIED: scripts/library.cmake:88-92]. The awk reducer in `scripts/library.sh` addresses the record by position and rejects anything else: `NF != 25 || !hex40($2) || !ok($3, "sms sms1 gg") || ... || !ok(src, "none db header heuristic default")` with `kv($10, "verdict600")` through `kv($25, "source")` [VERIFIED: scripts/library.sh:124-131]. To add words: append them after `source=` in `diag_print`; raise the `NF` constant (25 + number of words); read `$26...` with `kv()`; validate each with `ok(word, "<closed list>")`. Closed lists stay: `kinds = "none unmapped_read unmapped_write rom_write mapper_range ed_undefined unsupported_video_mode unknown"`, `devs = "none cpu cart ram mapper ctrl vdp unknown"`, `wheres = "rom slot2 ram mapper port cpu vdp unknown none"` (`library.sh:85-87`); leave them intact because the kind constants stay (a dropped kind can only stop appearing).

**Recommended placement (Claude's discretion):** put the new words in the per-record line (so they are validated) and print them in the reducer as **sub-rows** of the leaf, not as new per-row fields. The existing sub-row mechanism is `subs[g SUBSEP kind SUBSEP value]++` printed as `diag-group: leaf=<g> pcslot=|where=|pc= rows=N` with `split("pcslot where pc", label, " ")` (library.sh:154-156, 168, 173). Add labels (`faddr`, `vdpmode`, `wclass`, `pclass`, `size`, `header`) and single-value compound keys for the crosses (`faddr=fffc:y`, `vdpmode=text:sms`, `pclass=3e-3f:gg:export`; joined by `:`, because a `.` before a machine token would fire the leak scan's `\.(sms|gg)` pattern). This keeps counts-only output, adds no per-row identifying text, and needs one alternation extension in the grammar test regex. Sort keys: the existing `G` line sort uses a numeric sub-key (`a[2]`) then `a[3]`; give new labels sub-keys 4+ so order stays deterministic (the `taxonomy` test pins leaf order and byte-order of `diag:` lines).

**Tests to extend (and why each breaks otherwise):**
- `tests/runner/diagnose.cmake:6-14`: the `shape` regex is anchored at `source=(none|db|header|heuristic|default)$`; append the new words. `want_checked` stays 6 (or 9 with fixtures).
- `tests/library/diagnose.cmake`: (a) the `shape` alternation at lines 141-145 (`diag-group: leaf=[a-z_.]+ (pcslot=[0-4] |where=[a-z0-9]+ |pc=[0-9A-F]+ )?rows=[0-9]+`) must admit the new sub-row labels; (b) the `taxonomy` case writes 17 canned `diagnose 1 ...` lines (lines 190-205) that each must gain the new words (use one `set(tail ...)` variable), while keeping the three intentionally malformed rows (`mapper=bogus`, `extra=1`, missing `source=`) malformed: `malformed=3` and `evaluated=19` are asserted at line 222; (c) the `leak_title` case's single canned line (line 265); (d) the `hostile` case relies on extra output making `NF` wrong, so keep the check strict; (e) add one expectation per new sub-row kind (`diag_expect(... "leaf=... faddr=fffc:y rows=N")`).
- `docs/runner.md` Diagnose table and example line (lines 411-432) and `docs/conformance.md:30-34`.

**`ed-00` and `in-a-00` interplay:** the diagnose tests use `in-a-00.sms` as the "port fault" cart (library `main` case, `port.sms`, expecting `group=fault_no_display.unmapped_read.ctrl`, `leaf=... pcslot=0`, `where=port`) and `ld-8000.sms` for `rom_write` (`tests/runner/diagnose.cmake:62-64`). After D-04 both seeds produce `fault=none`. Keep the seeds (fuzz coverage): flip the runner assertions to `fault=none` (they become negative controls) and point the library `main` "fault row" at `ed-00.sms` (`fault_no_display.ed_undefined.cpu`, `where=cpu`, `pcslot=0`, since the seed is `ED 00 F3 76` at `$0000`). The `where=port` vocabulary stays covered by the canned `taxonomy` lines.

## 4. The base-versus-head pin proof (D-12), confirmed

`runner.hashes_file` runs `cmake -DCASE=hash_matrix ... -DOUT=${PROJECT_BINARY_DIR}` which calls `cmake -DMODE=write -DRUN=... -DOUT=... -DFIXTURES=... -P tests/cmake/hashes.cmake` and writes `${PROJECT_BINARY_DIR}/hashes.txt`, i.e. `build/ci/hashes.txt` under the `ci` preset [VERIFIED: tests/runner/tests.cmake:80-83,1613-1620; tests/cmake/hashes.cmake:1-4,127-129]. The same test also compares the sound rows with the frozen pins in `tests/audio/transitions.txt` and `tests/audio/pcm.txt`, so a moved audio pin already fails a ctest case; the frame rows ("frame 60 sha256", the `playing ...` rows, `mailbox ...` rows, the fixture digests) have no in-repo frozen copy except the test cards and `tests/diag/expected.csv`, which is why the base-versus-head compare is needed. `build/ci/hashes.txt` is 173 lines today (from an earlier local run; regenerate, do not trust it).

Cheapest exact commands, from the repository root (rule 11: no `cd` chains; `cmake -S` with `--preset` works, verified with `cmake -S <repo> --list-presets` run from another directory):

```sh
git worktree add --detach "$TMPDIR/p14-base" origin/main
cmake -S "$TMPDIR/p14-base" --preset ci
cmake --build "$TMPDIR/p14-base/build/ci"
ctest --test-dir "$TMPDIR/p14-base/build/ci" -R '^runner\.hashes_file$'
# head, after the last core commit:
cmake --preset ci && cmake --build --preset ci && ctest --preset ci -R '^runner\.hashes_file$'
cmake "-DFILES=$TMPDIR/p14-base/build/ci/hashes.txt;build/ci/hashes.txt" -DCOUNT=2 -DMODE=compare -P tests/cmake/hashes.cmake
```

The compare passes only for exactly `COUNT` existing byte-identical files and names the first that differs (`hashes: ... differs from ...`). Exit status 0 is the evidence; record the command, the status and (rule 11) the run URL of the CI run for the same head, never the files. Shortcut that gives the same bytes: `git diff --stat origin/main HEAD -- src include runner scripts tests` is empty at research time (HEAD differs from `origin/main` only under `.planning/`), so plan 1 may capture `build/ci/hashes.txt` from the unchanged tree instead of building a worktree; the worktree form above is the locked wording and is the one to use if any `src/` commit already exists. The `hash-equality` CI job compares lanes with `-DCOUNT=8`, not base against head; it is no substitute.

D-06 and D-08 are the only fixes that can make this compare fail. If either fires, name each moved line, justify it, and the revision is already 18.

## 5. The boots check (FIX-02): where it is keyed

- `boots@600` is the string in the ledger configuration: `scripts/compat-common.sh:77` rejects a row whose `parameter!="boots@600"` (the same line checks `f[8]~/^(pass|fail)$/`); `scripts/library.cmake:151` selects rows with `"${${case}_parameter}" STREQUAL "boots@600"`, `:261` writes the judged expect file `boots@600\n`, `:273` writes `.../boots@600` into the public-facts evidence column; `scripts/compat-seed.sh:17,29` builds the key `...|boots@600`; `scripts/library.sh:185` filters ledger fails on `L[n, "parameter"] != "boots@600"`. `tests/compat/ledger.csv` has 1,977 lines containing it out of 1,987 [VERIFIED: grep counts this session]. Moving the cap changes every row's identity, so each row would need migration or an exception; D-09's claim holds.
- `cogmaster__expect_boots(e)` is `return e->display && e->distinct >= 2u && !e->fault;` [VERIFIED: runner/expect.c:181-184]. `distinct` is 1 after the first frame and becomes 2 when any later frame's crop hash differs from frame 1's (expect.c:172-177): a cart whose picture is unchanged from frame 1 fails, which is the static-display rule. `fault` latches `fault.present` on any frame, so after D-04 the verdict stops failing on benign faults with no change to this function.
- The Phase 13 first-change histogram supports D-09 at bucket resolution: sega last bucket `241-300` (5 rows), none `361-420` (1 row), nothing past 420 [VERIFIED: 13-CAUSE-GROUPS.md:53-73]. The histogram is in 60-frame buckets, so "a gap" is a gap of buckets (301-360, then 421-600).
- Fixtures that stay: `static-display.sms` and `di-halt.sms` seeds (`tests/fuzz/seeds.c`), asserted in `tests/runner/expect.cmake:80` (`display=1 distinct_at_least=1 first_fault=0`) and `tests/runner/diagnose.cmake:61,66`. If the cap never moves, FIX-02 needs no fixture (roadmap criterion 2: "if it shows the check is sound, that is the stated reason and the existing never-draws fixture stays").
- The `static_no_fault` pad-press run (D-14): `--diagnose` refuses `--input` (exit 2, `tests/runner/diagnose.cmake:77-86`), so the run is an ordinary capped run: `cogmaster-run --frames 600 --input <script> --expect <file with boots@600> <file>`, exit 0 means pass, 3 means fail (`docs/runner.md:395-407`). The script is 600 records `p1=00 p2=00 pause=0 reset=0 start=0` with button bits (bit 4 is button 1) set from some frame; write both files to the scratch directory, never the tree. The agent finds the cart by looping `cogmaster-run --probe` over the local directory and matching `title=`; nothing from that is written to a file. Record only the two exit statuses.

## 6. The Sega bank shift and the `$FFFC` tests (D-05, D-06)

- Shift is applied unconditionally: `p->bank[addr - 0xFFFDu] = bank_write(m, addr, (uint32_t)v + shift[p->ctrl & SEGA_SHIFT]);` with `static const uint8_t shift[4] = {0x00u, 0x18u, 0x10u, 0x08u};` [VERIFIED: src/mapper.c:115-120]. `bank_of` reduces modulo `m->cart.pages` (src/mapper.c:30-34).
- **Existing coverage of the shift:** `tests/mapper/test_mapper.c:233-248` `test_sega_shift` (registered as `sega.shift`, line 517) uses a 512 KiB (32-page) image; the guest fixture `tests/roms/src/mapper_sega.asm` is 512 KiB ("`BANKSTOTAL 32`") and its record `0A` (`bank_shift`, expected `0918` in `tests/diag/expected.csv`) writes `$FFFC = 1` and `$FFFC = 3`. D-06's proposed gate ("images of 32 pages or more") leaves both unchanged. Any fixture of 8 pages or fewer that writes a nonzero shift would move; `tests/fuzz/seeds.c` `fffc-writes.sms` (64 KiB) writes `$FFFC = 0` only.
- **`$FFFC` behaviour tests today:** `tests/bus/test_bus.c:368-386` `test_fault_mapper_range` (a 384 KiB image `LD A,1E; LD ($FFFF),A` faults; bank 23 of 24 does not; bank 35 of 32 wraps; then `bad[] = {0x80,0x10,0x01,0x02,0x03}` each fault on a 512 KiB image; `$0C` does not), `tests/mapper/test_mapper.c` cart-RAM cases (`$FFFC = $08/$0C`, lines 175-205, 444-458), `tests/mapper/test_eeprom.c:676-708`. The fixture helper is `load_sega(prog, n_prog, n)` (`tests/bus/test_bus.c:84-95`): an `n`-byte image of `$FF`, the program at `$0000`, the `TMR SEGA` header magic at `$7FF0`, and `CHECK_EQ_U64(m->cart.board, COGMASTER__BOARD_SEGA)`; `big[BIG_MAX]` is 512 KiB. `test_bus.c` includes `internal.h`, so a test can read `m->mapper.ctrl` and `m->mapper.bank[]` directly.
- **D-06 trace:** one `cogmaster-run --trace first-fault` on an exemplar of the `fffc` bucket prints `fault frame=... pc=$.... op=$32 addr=$FFFC ...`; the value is not in the line (`addr` is the address, not the data). To see the data written, use `--frames N --trace first-fault` just to learn the PC, then read the guest's code at that PC from the local image (an agent shell step, nothing recorded), or run `cogmaster-run --frames 3 ... ` with `cogmaster_peek` through the `--expect ram@...` predicate at `$DFFC` (work RAM mirrors the register, `docs/boards.md` "Every register write also lands in work RAM, so `$DFFC`-`$DFFF` read them back"): `--expect` with `ram@3:$DFFC=BB` tests a guess byte by byte; a loop over 256 values of `ram@F:$DFFC=XX` finds the value with exit statuses only. Do not print the byte into a file; recording only "bits 1-0 set: yes or no" satisfies the outcome note.

## 7. Hardware anchors (quote these; flag the gaps)

| Claim | Where it is documented | Quote / anchor | Supported? |
|---|---|---|---|
| `$FFFC` b7, b4, b1-0 unused | SMS-CARTRIDGES-AND-PERIPHERALS.md section 2.1 table, lines 61-66 | "$FFFC b7 \| "ROM write" enable ... no software uses it"; "$FFFC b4 \| on-cart RAM over $C000-$FFFF; no known software uses it"; "$FFFC b1-0 \| bank shift ... (one 512 KB chip; no software uses it)" | Yes for "no software uses them"; the documents do not say a write to them is *inert* on retail parts: only that the registers exist on some chips (ares models b7). D-05's "inert on retail hardware (SMS Power mappers table)" is a mild extension; tag the outcome row as selected model, not measured. |
| `$FFFC` also in SMS-SYSTEM | SMS-SYSTEM.md section 3 row `$FFFC` | "bit 7 ROM write enable, 4 RAM at $C000-$FFFF (needs $3E bit 4 set; unused by software), 3 RAM in slot 2, 2 RAM bank, 1-0 bank shift ($00/$18/$10/$08; one 512 KB chip only)" | Yes |
| Banks wrap modulo pages | DECISIONS.md DEC.37; SMS-CARTRIDGES 2.4 | "images are padded to a 16 KB multiple and banks wrap modulo pages"; "Power-of-two ROMs mirror, so over-range bank numbers are harmless" | Yes for wrap; **no** document says odd-size over-range is a fault (Premise 6) |
| Reads of `$00-$3F` are open bus | SMS-SYSTEM.md C5 and section 4 | "Reads of unused ports $00-$3F on SMS1 are "open bus", $FF on SMS2"; table row "$00-$3F even ($3E) \| SMS1 open bus; SMS2/GG $FF" | Yes |
| Writes to undecoded ports | SMS-SYSTEM.md section 4 | "on the Japanese SMS, Korean units, Mark III and Game Gear the $3E/$3F mirrors across $00-$3F do not exist" | **Partly**: says the mirrors do not exist, says nothing about the write's effect (Premise 5) |
| A ROM has no write line | not documented | none | **No**: this is general hardware knowledge [ASSUMED]; the supporting statement in the documents is the mapper tables listing only exact registers (2.1, 2.2). The decision table should say "no document lists a fault; the mapper tables name exact registers only". |
| Mode select nibble | SMS-VDP.md 2.5 | "(M4 M3 M2 M1): 0xxx = TMS modes (7). 1000, 1010, 1100 = mode 4 at 192 on every chip. 1011 = 224 and 1110 = 240 on SMS2/GG, the invalid text mode on SMS1. 1111 = mode 4 on SMS2/GG, invalid text mode on SMS1. 1001 and 1101 = invalid text mode everywhere" | Yes; matches `vdp_supported` exactly |
| Legacy modes exist, no renderer | SMS-VDP.md section 7 ("later seed; facts recorded now"); DEC.03 | "Modes on the SMS: 0 Graphics I ..., 1 Text ..., 2 Graphics II ..., 3 Multicolor ..., plus the four undocumented mixes"; DEC.03 "SG-1000 and SC-3000 come after v1.0 (SEED-006)" | Yes for "not emulated by design"; section 7 is a summary only (no datasheet text) |
| Open bus on port reads is "already the documented principle" | docs/embedding.md:361-366 | "Things shipped games do are never faults ... An undriven read returns `$FF` and counts in the instance's open-bus counter instead" | Yes |
| First fault is captured | DEC.39, DEC.40 | "the first fault (PC, opcode, address, width, device, frame, line, dot) is captured from phase 1" | Yes |

## 8. Docs that must change (and budgets)

All sizes are bytes now against `docs.max_bytes` 38,912 (`NOT size LESS cap` fails, so each file must stay below 38,912) [VERIFIED: quality.json; tests/cmake/budgets.cmake:10-24; `wc -c`]:

| File | Size | Headroom | Sentences that become wrong |
|---|---|---|---|
| docs/embedding.md | 36,442 | 2,470 | Line 322-324 intro ("an access to an unmapped address or port, a write to ROM, a mapper value out of range, or an undefined opcode"); table rows 1 (the `$00-$3F` port clause), 2 (the full-decode port clause), 3 (whole row: no longer recorded), 4 (the `$FFFC` clause), 6 (add: `addr` carries the mode nibble M4 M3 M2 M1); the paragraph at 355-359 ("`addr` is the full 16-bit bus address") needs an exception for kind 6; the "never faults" list at 361-366 gains: writes to a ROM address that is no register, writes to a port nothing decodes, reads of `$00-$3F`, writes to `$FFFC` unused bits. One revision sentence. The kind constants stay listed ("both lists only grow"): say rows 3 and the clauses above are "no longer recorded". |
| docs/runner.md | 36,916 | **1,996** | Line 218 example (`kind=unmapped_read` for `IN A,($00)`, no longer produced: use an `ed_undefined` example); line 233 `kind` row stays valid; the Diagnose table and example (411-432) gain the new words (budget-critical: about 1,400-1,900 bytes available, so write compact rows and trim elsewhere if needed). Line 395-396 (`boots@F` "no first fault was recorded") stays true. |
| docs/conformance.md | 35,312 | 3,600 | Lines 30-34 (what diagnose prints): add the evidence classes in one sentence. |
| docs/testing.md | 35,039 | 3,873 | Lines 16-26 list the `library.diagnose*` tests: extend only if a case is added. |
| docs/maintaining.md | 19,862 | 19,000 | No wrong sentence found by grep (`diagnos`, `fault`, `rom_write`); touch only if a maintenance procedure for the diagnose words is wanted. |
| docs/boards.md | 9,221 | 29,000 | Line 91 ("a register write past the end also records a `mapper_range` fault") stays true. The per-board "Writes ... change nothing" lines already match. No change needed. |
| docs/hardware/video.md | n/a (not in the `docs/*.md` budget glob) | | Line 74 ("record `COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE` on the VDP"): add the nibble sentence; lines 55-57 only if D-08 fires. |
| docs/controllers.md | 21,358 | | Lines 178-181 only if D-08 fires. |

Gates on docs: `docs.api_names` requires every backticked public name in `docs/embedding.md`, `saves.md`, `boards.md` to exist in the header or build and every header name to appear in the guide [VERIFIED: tests/policy/tests.cmake:122-131]; a claims check rejects planning labels (e.g. "D-04", "FIX-02", "SEED-006") in user text [VERIFIED: tests/policy/claims_cases.cmake:55,67]. Docs must say "selected model", never "measured" (CONTEXT specifics).

## 9. SEEDs and the outcomes note

- **SEED-016** (`.planning/seeds/SEED-016-failing-carts.md`): frontmatter `status: dormant`, sections "Why This Matters", "What the ledger already shows", "Where to start", "Open question for the discuss step"; it has no Breadcrumbs section. Add a short `## Outcome` or `## Breadcrumbs` list pointing at `14-FIX-OUTCOMES.md` and the closed groups.
- **SEED-006** (`.planning/seeds/SEED-006-sg1000-mode.md`): has `## Breadcrumbs` (three preparation paths). Append the D-07 line "the library shows N rows needing legacy modes" with N copied from the final `vdpmode` count (counts only, no title), and the SEED's own "When to Surface" sentence ("small enough to ride inside v2.0 if the VDP's legacy modes are already needed for an SMS title in the ledger") is the trigger it satisfies.
- **`14-FIX-OUTCOMES.md` header to copy from `13-CAUSE-GROUPS.md:1-11`:** title line, a "Counts only." sentence, then a bullet list: `core commit:` (full `git rev-parse HEAD`, run on a committed tree: `scripts/library.sh` prints `core=<7 chars>[+dirty]` and marks dirty when a tracked file outside `.planning` differs from HEAD), `behaviour revision:`, `date (UTC):` (`date -u +%F`), `command:` (``sh scripts/library.sh diagnose``, exit status), `ledger read:` (commit of `tests/compat/ledger.csv`, `git log -1 --format=%H -- tests/compat/ledger.csv`), `caps:` ("verdict at frame 600, view to frame 1800"), plus `phase 13 note:` (path and commit). Then the D-03 decision table, then the D-14 table `rank | leaf | rows (13 note) | outcome | evidence | residual leaf counts`, with ranks and leaf names exactly as in 13-CAUSE-GROUPS.md (`fault_alive.mapper_range.mapper`, `fault_static.unsupported_video_mode.vdp`, `fault_alive.rom_write.cart`, `fault_alive.unmapped_write.none`, `fault_alive.unmapped_read.ctrl`, `fault_alive.unsupported_video_mode.vdp`, then the under-5 groups as printed). Never write a title, hash, path or address; the leaf counts and class words are the evidence.
- The baseline of the new classes for the *benign* kinds exists only in plan 1's run: after D-04 those kinds are never recorded, so their address classes cannot be re-measured. Copy the baseline class counts into the outcomes note in plan 1 before any kind is dropped.

## 10. Test edit map (existing assertions on dropped kinds)

Found by `grep` for each kind constant, each kind name, and `present` / `fault_present` assertions across `tests/`, `docs/`, `scripts/`, `runner/`. Each row names what changes. "Positive" is the control that keeps firing.

| File:line | Today | Change |
|---|---|---|
| tests/bus/test_bus.c:279-300 `test_fault_unmapped_read` | `LD A,0; IN A,($00); HALT` expects kind 1 device 5 | Becomes the **negative** control: `present == 0`, `A == $FF`, `open_bus_reads == 1`. Positive for kind 1 already exists in `tests/io/test_io.c:92-130` `test_port3e` (cart/RAM off via `$3E`, memory side). |
| tests/bus/test_bus.c:335-346 `test_fault_unmapped_write` | `OUT ($3E)` on GG and `OUT ($00)` on the Japanese console expect kind 2 | Both become `expect_none`; the export case already is. Positive for kind 2 is `tests/mapper/test_mapper.c:449-499` (cart or RAM off) and `tests/ports/test_glasses.c:316`. |
| tests/bus/test_bus.c:353-361 `test_fault_rom_write` | three `expect_fault(... 3u, 2u, ...)`-style cases | `expect_none` for `LD ($2000),A` on a Sega image after `LD ($FFFF),A` and on a ROM-only image; add the positive control that the register took effect (`m->mapper.bank[2]` equals `0x12 % pages` after the frame). The cart-RAM `expect_none` stays. |
| tests/bus/test_bus.c:368-386 `test_fault_mapper_range` | `bad[]` loop expects a `$FFFC` fault on 512 KiB | The loop becomes `expect_none` plus an assertion `m->mapper.ctrl == bad[i]` (the value is stored); keep the 384 KiB `0x1E` fault (positive) and the bank-wrap `expect_none`s. If D-06's gate fires, a fixture of 8 pages with `$FFFC = 1` then `$FFFF = 2` asserts the shift is not applied below 32 pages. Table entries at lines 512-517 and CTest names `fault.*` (tests/bus/tests.cmake:47,54) stay, so `tests/inventory/ci.txt` and `wasm32.txt` need no edit. |
| tests/io/test_io.c:226-240 `test_gg_sms_mode` | line 239 `fault_of(m).kind == UNMAPPED_WRITE` after the `$02` mirror write | `fault_of(m).present == 0`. |
| tests/mapper/test_mapper_ext.c:258, 294, 405 | `expect_fault(... COGMASTER__FAULT_ROM_WRITE, COGMASTER__DEVICE_CART, ...)` | Each becomes `CHECK_EQ_U64(fault_of(r.m).present, 0)`; the `MAPPER_RANGE` assertions at 261 and 367 stay (positive). The `seen_over` bookkeeping stays, but note that a Korean no-bank `v == 0` branch (258) has no first fault now, so the following `else if (seen_over)` ordering must not assume the ROM-write fault came first. |
| tests/mapper/test_eeprom.c:702, 707 | `fault_present(&r) == 1u` after a ROM write and after a closed-window write | `0u`; line 697 (bank 5 of 3 pages) stays `1u` (positive). Update the comment at 676-678 ("and a write to ROM does"). Not named in D-13. |
| tests/api/v2.c:117-141 | `LD A,0; IN A,($00); HALT` then `kind == UNMAPPED_READ`, `device == CTRL` | Replace the program with `ED 00 ...` and assert `COGMASTER_FAULT_KIND_ED_UNDEFINED` / `COGMASTER_FAULT_DEVICE_CPU` (constants asserted just above), and update the comment. Not named in D-13. |
| tests/core/test_core.c:435-436 | revision comment and `17u` | 18 (section 2). |
| tests/runner/expect.cmake:79 | `check(3 "first_fault=1.*fault frame=.*kind=unmapped_read" ... in-a-00.sms)` | Point at `ed-00.sms` with `kind=ed_undefined`. |
| tests/runner/tests.cmake:1020-1023 (`trace_fault`) | `in-a-00.sms` with `op=[$]DB addr=[$]0000 dev=ctrl kind=unmapped_read` | `ed-00.sms`: from `src/bus.c:346-353` the record is `pc = z80.pc - 2`, `addr = z80.pc - 1`, `op` = the second byte, `dev=cpu kind=ed_undefined`, so `pc=$0000 op=$00 addr=$0001`. The following `unsupported-video` case at 1025-1031 stays (positive for kind 6). |
| tests/runner/tests.cmake:1043-1045 (`png_unreached`) | `in-a-00.sms --trace first-fault` stops after the first frame | Use `ed-00.sms` (faults in frame 0, so the stop happens after frame 1). |
| tests/runner/diagnose.cmake:62-64 | `in-a-00` -> `fault=unmapped_read ... where=port`; `ld-8000` -> `fault=rom_write ... where=slot2` | Flip to `fault=none ...` (negative controls: the diagnose line proves the kinds are no longer recorded); keep `ed-00` (positive). |
| tests/library/diagnose.cmake:105, 152-155 (the `main` case; the `taxonomy` case at 190, 219, 233 uses canned `unmapped_read` text and stays) | `port.sms` from `in-a-00` expecting `fault_no_display.unmapped_read.ctrl`, `where=port`, `pcslot=0` | Use `ed-00.sms`; expectations become `fault_no_display.ed_undefined.cpu`, `where=cpu`, `pcslot=0`. `candidates=6 attempted=4 ... judged=4 run_fail=3` stay. |
| tests/vdp/test_background.c:387-410 | each unsupported nibble faults on VDP | Add `CHECK_EQ_HEX(f.addr, mode)` (the loop's `mode` is the nibble by construction of `reg[0]`/`reg[1]`). Positive: every unsupported nibble on both profiles. Negative: add `present == 0` for the `valid` modes (currently `continue`d). |
| tests/ports/test_gun.c:907-920 | `memcmp(&g->fault, &p->fault, ...)` and `fault.line == 0`, `fault.dot == COGMASTER__FIELD_W` | Unchanged for the nibble (both runs record the same `addr`); changes only if D-08 moves the raise (section 1). |
| tests/core/test_state.c | feeds `sample_fault()` directly | **No change** needed. Optionally extend `sample_fault` so a recorded `addr` below 16 round-trips; it already round-trips `0xBEEF + seed`. |
| tests/diag/expected.csv | no fault rows | **No change**. |
| tests/bios/test_bios.c:130-155, 224 | memory-side `open_bus_reads == fault_present` and `fault_present == 1` for a no-BIOS slot read | **No change** (memory side stays). |
| tests/audio/test_opll.c:908,938,960; tests/ports/test_gun.c:1017 | `fault_present == 0`, open-bus counter parity | **No change**; the counter still increments at `io.c:465`. |

CTest names that must not be removed or renamed (the inventory fails configure otherwise, `tests/inventory/ci.txt` line 2: "Sorted; configure fails when this differs from what is registered"; `quality.json` floors `tests.ci` 669 and `tests.wasm32` 627): `fault.unmapped_read`, `fault.unmapped_write`, `fault.rom_write`, `fault.mapper_range`, `fault.not_faults`, `runner.diagnose`, `runner.diagnose.class`, `library.diagnose*`. Edit bodies, not names. A new CTest case needs an inventory line (and `wasm32.txt` for `portable` ones) and is not required by D-13.

## 11. Suggested plan shape (consistent with D-01, no decision reopened)

1. **Plan 1 (diagnostic, nibble, baseline, revision):** runner classes + `diag_print` words + `diag_class.h` pure functions + `runner.diagnose.class` cases; `scripts/library.sh` reducer sub-rows and validator; `tests/runner/diagnose.cmake` and `tests/library/diagnose.cmake` extensions; `src/vdp_render.c` nibble + `vdp_mode` helper; revision 17 to 18 with the test_core pin and the `docs/embedding.md` sentence; `docs/runner.md` and `docs/conformance.md` word text; baseline `diagnose` run (owner exports `COGMASTER_ROM_DIR`; must print `run_fail=205`, `run_fail_not_in_ledger=0`); `14-FIX-OUTCOMES.md` with the header, decision table and baseline class counts. Gate: `cmake --workflow --preset ci`.
2. **Plan 2 ($FFFC, rank 1, also rank 8):** delete the raise and `SEGA_UNUSED`; the D-06 trace decides the gate (default: leave and say why); edit `test_bus.c` `test_fault_mapper_range`, `docs/embedding.md` row 4; re-run diagnose; outcome row.
3. **Plan 3 (rom_write, ranks 3 and 7):** delete the 8 `rom_write` raises; the evidence trigger (foreign-register class) is read from plan 1's baseline, not re-measured; edit the `rom_write` tests and the seed repoints (`ld-8000` assertions); outcome rows.
4. **Plan 4 (ports, ranks 4 and 5):** delete `io.c:463` and `io.c:508` raises; check the port-class baseline for the D-11 stop condition (mirror writes on a Japanese full-decode profile) first; edit `test_bus.c`, `test_io.c`, `tests/api/v2.c`, the `in-a-00` repoints; outcome rows.
5. **Plan 5 (video closures and the D-08 decision):** rank 2 closed with the D-07 reason and the `vdpmode` counts, SEED-006 breadcrumb; rank 6 decided by evidence (fix and repoint, or close).
6. **Last plan:** FIX-03 families (zemina, korean_nb, korean) from the cross-cut times `vdpmode` and `size`; boots-check record (D-09) with the two no-fault rows settled from `distinct1800` and first non-blank, `static_no_fault` pad-press run; the base-versus-head pin compare; docs sweep; final `diagnose` and the revision paragraph; SEED-016 outcome pointer.

Ordering hazards: (a) the record keeps the first fault only, so each drop can expose the next fault in the same cart and a leaf can grow (a row that left `fault_alive.rom_write.cart` may land in `fault_static.unsupported_video_mode.vdp` or `static_no_fault.*`); judge each fix by its own leaf shrinking, not by flips. (b) A drop can only turn a fail into a pass or a fail with another first fault, never a pass into a fail, because the boots verdict depends on pixels (unchanged) and `!fault` (the fault set only shrinks): `run_fail_not_in_ledger=0` is guaranteed by construction for D-04, D-05 and the nibble, and is a real check only for D-06 and D-08. (c) The `diag` classes for the benign kinds vanish with the kinds; capture them first (section 9). (d) Edits to `tests/runner/diagnose.cmake` and `tests/library/diagnose.cmake` occur in both plan 1 (grammar) and plans 3 and 4 (seed repoints); put the seed repoint in the same plan as the raise it depends on so every commit's ci stays green.

## 12. Common pitfalls

### Pitfall 1: a boundary-class word that breaks the leak scan
**What goes wrong:** a class word with a slash, `.sms`/`.gg`, or eight hex characters fails `library.diagnose.leak_*`.
**How to avoid:** closed lists in awk, lower-case letters, digits and `-` only, no value-derived text.
**Warning signs:** `diagnose leak: leak_digest` / `leak_path` / `leak_name` from `tests/library/diagnose.cmake`.

### Pitfall 2: tests that rely on the first-fault being the ROM write
**What goes wrong:** `tests/mapper/test_mapper_ext.c` asserts `expect_fault(...0x1234u)` after a series of writes; with the raise gone the first fault in that rig may be a later `mapper_range`, and `check_banks`/`present == 0` assertions may then see an unrelated fault from the sweep.
**How to avoid:** read the loop before editing (section 10): the ROM-write assertions sit after a `COGMASTER_RESET_POWER` that clears the record, so `present == 0` there is sound; the loops above them use `seen_over` to decide.

### Pitfall 3: the runner shape regex and `NF` are strict by design
**What goes wrong:** forgetting one canned line in `tests/library/diagnose.cmake` `taxonomy` makes it count as `malformed`, shifting `evaluated`, `judged`, `run_fail`, `groups_total` and every `diag_expect`.
**How to avoid:** one shared `tail` variable appended to every canned line; keep exactly the three malformed rows.

### Pitfall 4: `where=port` and `dev=ctrl` become unreachable from the live core
After ports are dropped, no live fault has device `none` or `ctrl`; the vocabulary must stay (closed lists, docs, `taxonomy` canned lines) because the lists only grow and an older record must still validate.

### Pitfall 5: moving `$FFFC` fault removal into the shift gate by accident
Removing the raise changes no bank; adding the D-06 gate does. Keep them in separate commits so a `hashes.txt` compare failure names the gate.

### Pitfall 6: nibble 0 and "no information"
`addr = 0` was the old value for kind 6; nibble 0 (Graphics I) is now also `0`. That is harmless (the kind says it is a VDP fault, the nibble is the mode) but the docs must say the nibble is meaningful for kind 6 only and `0` is a valid value.

### Pitfall 7: SMS1 rows
The library maps a catalog machine of `sms1` to `--machine sms1` (library.cmake profile regex `^sms1?$`), where nibbles 11, 14 and 15 are invalid. Read the `invalid` class crossed with machine before calling any row an M4 decode bug.

### Pitfall 8: doc budgets and docs gates
`docs/runner.md` has about 2 KB of headroom; the new words need compact table rows. `docs.api_names` and the planning-label claims check apply to every doc edit.

### Pitfall 9: evidence rules
Evidence is a command, its exit status and a run URL (rule 11); never commit a log, a title, a path or an address. `core=<sha>+dirty` runs are not citable: commit first, then run.

## Don't Hand-Roll

| Problem | Don't build | Use instead | Why |
|---|---|---|---|
| Pin proof (base vs head) | A new comparison test | `runner.hashes_file` + `tests/cmake/hashes.cmake -DMODE=compare` | Already written; adds no repository-check lines |
| Per-kind filter in the boots predicate | A skip-list in `runner/expect.c` | Drop the raise site in the core | The record keeps only the first fault; a runner filter would hide a later real fault |
| Mode nibble in the record | A new public field or VDP register peek | `addr` (u16) | No ABI or state-format change |
| New seeds | New `.asm` fixtures or committed binaries | Inline byte arrays in tests; existing seeds from `tests/fuzz/seeds.c` | No manifest entry, no hygiene exposure |
| Evidence output | A second diagnostic tool | Extend `runner/diag.c`, `diag_class.h`, the awk reducer | Leak scan and validator already cover it |

## Code Examples

### Dropping a benign raise, keeping the counter (src/io.c, port read)
```c
/* Source: src/io.c:451-465 (current) -- the shape after the change */
    if (p < 0x40u) {
        /* $3E and $3F are write-only; nothing drives a read there. */
    }
    return cogmaster__io_open_bus(m);
```
Delete the empty `if` and keep the comment attached to the return (SMS-SYSTEM C5: reads of `$00-$3F` are open bus).

### The nibble in the record (src/vdp_render.c)
```c
/* Source: src/vdp_render.c:54-63 (current mode expression), M4 M3 M2 M1 (SMS-VDP 2.5) */
static uint32_t vdp_mode(const cogmaster *m)
{
    const uint8_t *r = m->vdp.reg;
    return ((uint32_t)(r[0] & 4u) << 1) | ((r[1] & 8u) >> 1) | (r[0] & 2u) | ((r[1] & 16u) >> 4);
}
/* ... in cogmaster__vdp_render_until: */
        cogmaster__bus_fault(m, (uint16_t)vdp_mode(m), COGMASTER_FAULT_DEVICE_VDP,
                             COGMASTER_FAULT_KIND_UNSUPPORTED_VIDEO_MODE);
```

### A negative-and-positive pair in test_bus.c (shape)
```c
/* LD A,$12; LD ($FFFF),A; LD ($2000),A; HALT on a 128 KiB Sega image */
cogmaster *m = load_sega(sega, sizeof sega, 128u * 1024u);
(void)run(m);
CHECK_EQ_U64(fault_of(m).present, 0);                 /* no register, no fault */
CHECK_EQ_U64(m->mapper.bank[2], 0x12u % m->cart.pages); /* register still behaves */
cogmaster_destroy(m);
```

### Chosen-value class test (tests/runner/test_runner.c, no emulation)
Extend `diag_class()` with calls such as `cogmaster__diag_vdpmode(0u)`, `(1u)`, `(9u)`, `(13u)` and, for the machine cross, the SMS1 pairs; the planner names the function; the shape is the existing `cogmaster__diag_where` style (a `switch` returning a literal word).

## State of the Art

| Old approach | Current approach | Impact |
|---|---|---|
| First fault = any odd access | First fault = something no hardware answers | The boots verdict stops failing for ROM writes, undecoded ports and unused `$FFFC` bits |
| `address` 0 for kind 6 | the `M4 M3 M2 M1` nibble | A refusal is distinguishable from a bug |

## Runtime State Inventory

Not a rename or migration phase: omitted. (The only stored runtime state is the saved-state format, which refuses old revisions by design; the tree holds no stored state fixture.)

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | A ROM has no write line, so a write to a non-register ROM address has no hardware effect | Premise 5/7, section 1 | The decision table would cite a document that does not say it; the planner should phrase the benign class as "no document lists a fault" |
| A2 | The Korean/Zemina carts in the 36-row group use a TMS mode because they are MSX-style ports | Premise 7 | Row causes could differ (e.g. an `invalid` class from SMS1 profile); the `vdpmode` x machine counts settle it |
| A3 | `cogmaster-run --expect` with `ram@F:$DFFC=XX` can find the byte written by a guest to `$FFFC` through the work-RAM mirror | Section 6 | The D-06 trace would need another route (reading the guest code at the first-fault PC in the agent shell) |
| A4 | The `--expect` frame cap allows `boots@600` with a `--input` script of 600 records (the runner takes exactly one record per requested frame) | Section 5 | The static_no_fault pad-press run would need `--frames 600` to match the script length, which is what the section states |

## Open Questions (RESOLVED)

1. **Should the nibble edit live in plan 1 or in the plan that first needs it?** RESOLVED: the nibble edit, its test and the 17 to 18 bump ride with the evidence work in plan 02 (the second half of D-01's "Plan 1", split from plan 01 for context budget), before any plan that reads `vdpmode` counts and before any fix plan.
   - Known: `vdpmode` cannot be produced without it and the baseline should predate benign-kind drops; the revision bump is D-12's "first behaviour-changing plan".
   - Unclear: whether the owner reads D-01's "before any core edit" as excluding it.
   - Recommendation: plan 1 (Premise 1); it changes no fault kind, presence or frame.
2. **How does the planner want per-word crosses printed?** RESOLVED: as counts-only sub-rows of each leaf with single compound values joined by `:` (plan 01, the diagnostic plan); `:` rather than `.` because a dotted key ending in a machine token matches the leak scan's `\.(sms|gg)` pattern.
   - Recommendation: sub-rows with compound single values (section 3); no per-row field grows.
3. **Does `static_no_fault` need the pad press given `distinct1800` and `ram`?** RESOLVED: one pad-press run, exit status only, in the closure plan (plan 08); rows beyond one are closed by their own `distinct1800` (D-09) and the owner is asked once about extending the run, since a wider campaign is deferred.
   - The leaf name `static_no_fault.frozen` already means `distinct1800 < 2` and RAM unchanged between calls 1740 and 1800 (library.sh:148); the D-14 pad-press run stays as decided, a single exit-status run.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake / ctest | every gate, pin proof | yes | 4.4.3 | none needed |
| git, awk, sh, gh | worktree, reducer, PR | yes | system | none needed |
| `COGMASTER_ROM_DIR` (owner's dumps) | baseline and per-plan `diagnose` | unknown: not looked for (rule 1/12) | owner exports at execute start | D-02: land the documentation-grounded fixes, mark outcomes `fixed, unconfirmed on dumps` |

**Missing dependencies with no fallback:** none. The library workflow, `sh scripts/library.sh` with no argument, and `run` must not be executed (they overwrite the tracked ledger and page).

## Validation Architecture

(Requested explicitly by the orchestrator although `.planning/config.json` has `workflow.nyquist_validation: false`.)

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest over C test programs (`cogmaster-test-*`) and CMake `-P` scripts |
| Config file | `CMakePresets.json` (`ci` workflow: configure, build, test, package, test `ci-release`) |
| Quick run command | `ctest --preset ci -R '^(fault\.|sega\.|mapper\.ext\.|io\.|vdp\.|runner\.diagnose|library\.diagnose|core\.)'` after `cmake --build --preset ci` |
| Full suite command | `cmake --workflow --preset ci` |

### Per fix: the test that proves it, with positive and negative controls

| Fix | Test (file, case) | Positive control (still fires) | Negative control (no longer fires / still behaves) |
|---|---|---|---|
| Nibble in `addr` (plan 1) | `tests/vdp/test_background.c` `unsupported()` (registered via `tests/vdp/tests.cmake`) | every unsupported nibble on SMS2 and SMS1 records kind 6, device VDP, `addr == mode` | supported nibbles (8, 10, 12; and 11, 14, 15 on SMS2) record nothing |
| Nibble words | `runner.diagnose.class` (`tests/runner/test_runner.c:diag_class`) | chosen nibbles 0, 1, 2, 4, 3, 9, 13 map to the documented classes | 8, 10, 12 map to the defensive `mode4` class; machine cross separates `sms1` |
| Diagnostic grammar | `runner.diagnose` (`tests/runner/diagnose.cmake`) and `library.diagnose`, `.taxonomy`, `.leak_*`, `.hostile` | `ed-00` still prints `fault=ed_undefined dev=cpu where=cpu` and the new words; the taxonomy case prints the new sub-rows | `in-a-00` and `ld-8000` print `fault=none`; the malformed canned rows stay `malformed=3`; the leak cases still fail by name |
| `$FFFC` unused bits | `tests/bus/test_bus.c` `test_fault_mapper_range` (`fault.mapper_range`) | 384 KiB image, bank `0x1E` still records kind 4 at `$FFFF`; 48-page-style eeprom case `tests/mapper/test_eeprom.c:697` | `$FFFC = 0x80/0x10/0x01/0x02/0x03` on 512 KiB: `present == 0` and `mapper.ctrl == value`; bank 35 of 32 wraps with no fault |
| `rom_write` | `tests/bus/test_bus.c` `test_fault_rom_write`; `tests/mapper/test_mapper_ext.c` (zemina, korean, korean_nb, hicom, 4 PAK: the sweeps); `tests/mapper/test_eeprom.c:board_banks` | exact register writes still change banks (`check_banks` after every write, sweeps) | non-register writes (`$1234`, `$0000`, `$4000`, `$8000`, `$A001`, `$BFFF`, ...) leave `present == 0` and change no bank; cart RAM at `$8000` still takes writes |
| Ports | `tests/bus/test_bus.c` `test_fault_unmapped_read`/`_write` (`fault.unmapped_read`, `fault.unmapped_write`); `tests/io/test_io.c` `test_gg_sms_mode`, `test_port3e`; `tests/api/v2.c` | memory-side kinds still fire: `test_port3e` (cart/RAM off), `tests/mapper/test_mapper.c:449-499`, `tests/ports/test_glasses.c:316` | `IN A,($00)` on export: `present == 0`, `A == $FF`, `open_bus_reads == 1`; `OUT ($3E)` on GG and `OUT ($00)` on Japan: `present == 0` |
| Seeds repointed | `tests/runner/expect.cmake`, `tests/runner/tests.cmake` (`trace_fault`, `png_unreached`) | `ed-00.sms` gives `kind=ed_undefined`; `unsupported-video` still `dev=vdp kind=unsupported_video_mode` | `in-a-00` run with `boots@20` no longer reports `first_fault=1` |
| D-06 gate (only if it fires) | `tests/bus/test_bus.c` + `tests/mapper/test_mapper.c` `sega.shift` | 512 KiB: shift codes 1-3 add `$18/$10/$08` (`sega.shift` unchanged); `tests/diag/expected.csv` `mapper_sega 0A:bank_shift 0918` unchanged | 8-page image with `$FFFC = 1`: `$FFFF = 2` selects bank 2 |
| D-08 (only if it fires) | `tests/vdp/test_background.c`, `tests/ports/test_gun.c:test_fault_neutral` | a persistent unsupported mode across tick 600 still records kind 6 | a mode that changes back within one line records nothing; frame hashes equal base |
| Revision | `tests/core/test_core.c:435-436` | `v.behaviour == 18u` | the `>=` API pins still pass |
| Pins unmoved | `runner.hashes_file` on base and head + `hashes.cmake` compare (section 4) | audio rows equal `tests/audio/*.txt` (in the test itself) | compare exit 0 means no frame or fixture-digest line moved |

### Sampling Rate
- **Per task commit:** the quick `ctest` expression above for the touched area.
- **Per wave merge:** `cmake --workflow --preset ci`.
- **Phase gate:** `cmake --workflow --preset ci` green on the phase head, the pin compare exit 0, and the final `diagnose` printing `run_fail_not_in_ledger=0`.

### Wave 0 Gaps
None: the framework, the seeds (`tests/fuzz/seeds.c`), the helpers (`load_sega`, `expect_fault`, `expect_none`, `fault_of`) and the inventory exist. New CTest names are not required; if one is added, add it to `tests/inventory/ci.txt` and `wasm32.txt`.

## Security Domain

`security_enforcement` is `false` in `.planning/config.json`: section omitted. The one boundary that matters is the leak contract of the diagnostic (Pitfall 1), which the existing canary scan covers.

## Sources

### Primary (HIGH confidence, read this session)
- `src/mapper.c`, `src/mapper_ext.c`, `src/io.c`, `src/bus.c`, `src/fault.c`, `src/state.c`, `src/vdp.c`, `src/vdp_render.c`, `src/gun.c`, `src/internal.h`, `src/cart.c`, `include/cogmaster.h`
- `runner/diag.c`, `runner/diag.h`, `runner/diag_class.h`, `runner/expect.c`, `scripts/library.sh`, `scripts/library.cmake`, `scripts/compat-common.sh` (boots key), `tests/cmake/hashes.cmake`, `tests/cmake/budgets.cmake`, `tests/policy/repository-checks.txt`, `quality.json`, `CMakePresets.json`
- `tests/bus/test_bus.c`, `tests/bus/tests.cmake`, `tests/mapper/test_mapper.c`, `tests/mapper/test_mapper_ext.c`, `tests/mapper/test_eeprom.c`, `tests/io/test_io.c`, `tests/api/v2.c`, `tests/core/test_core.c`, `tests/core/test_state.c`, `tests/vdp/test_background.c`, `tests/ports/test_gun.c`, `tests/runner/diagnose.cmake`, `tests/runner/expect.cmake`, `tests/runner/tests.cmake`, `tests/library/diagnose.cmake`, `tests/fuzz/seeds.c`, `tests/roms/manifest.txt`, `tests/compat/allow.txt`, `tests/testcard_pins.cmake`, `tests/diag/expected.csv`
- `docs/embedding.md`, `docs/runner.md`, `docs/conformance.md`, `docs/testing.md`, `docs/boards.md`, `docs/hardware/video.md`, `docs/controllers.md`
- `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` 2.1, 2.2, 2.4; `SMS-SYSTEM.md` 3, 4, C4, C5, C7; `SMS-VDP.md` 1, 2.5, 7; `DECISIONS.md` DEC.03, DEC.37, DEC.39, DEC.40
- `.planning/phases/13-why-carts-fail/13-CAUSE-GROUPS.md`, `.planning/seeds/SEED-006-sg1000-mode.md`, `SEED-016-failing-carts.md`, `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`

### Secondary / Tertiary
None. No web source was needed; no external package or documentation lookup applies.

## Metadata

**Confidence breakdown:**
- Raise sites, tests, revision touch list, pin proof, docs sizes: HIGH, all read this session.
- Diagnostic extension shape: HIGH for where words go, MEDIUM for the exact sub-row grammar (planner's choice).
- Hardware readings: MEDIUM; Premise checks 4 to 7 say where the documents stop.
- Cause hypotheses (109 and 36 rows): LOW until the new words are run; this research saw no dump.

**Research date:** 2026-11-12
**Valid until:** 2026-12-12 (the tree is stable; line numbers hold until the first plan commit lands)
