# Phase 14: Fix the causes - Context

**Gathered:** 2026-11-12
**Status:** Ready for planning

<domain>
## Phase Boundary

Every cause group of 5 or more failing default-profile rows in `.planning/phases/13-why-carts-fail/13-CAUSE-GROUPS.md`
(ranks 1 to 6: 109, 36, 32, 8, 7, 6 rows) gets a recorded outcome, fixed in the core or closed with a stated reason, worked in
descending order of size and citing that note. The `zemina`, `korean_nb` and `korean` rows each get a stated cause. The
boots check is either corrected or recorded as sound (FIX-01 to FIX-04).

Not this phase: editing the ledger, the compatibility page, `ci.yml` or any timeout (Phase 15); a numeric target for
flipped rows; new boards the groups do not name; PAL or Japanese FM work; a TMS9918 renderer (see D-07).

</domain>

<decisions>
## Implementation Decisions

Method: the owner's standing fan-out boilerplate was applied. Five advisor agents (fault policy and the boots check; the
109-row `mapper_range` group; the `unsupported_video_mode` groups; mapper families, `rom_write` and port faults; phase shape
and verification) each read the code and the preparation documents, applied the role lenses and ran a self-adversarial pass.
The orchestrator reconciled them (conflicts are named below), checked their citations against the tree and ran a
cross-cutting pass. No advisor saw a dump. Their cause claims are hypotheses until the counts in D-02 confirm them; the
decisions are written so that each one names the count that settles it. All areas were taken, no per-area vote was held
(the owner's boilerplate asks for one coherent set that is followed), and the earlier decision that the agent runs the
diagnostic in a shell where the owner passes `COGMASTER_ROM_DIR` stands.

### Phase shape and evidence

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

### Fault policy (rom_write, ports, mapper_range at $FFFC)

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

### Video modes

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

### The boots check (FIX-02)

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

### Mapper families (FIX-03)

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

### Tests, pins and shipping

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

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase inputs
- `.planning/phases/13-why-carts-fail/13-CAUSE-GROUPS.md` - the counts every outcome cites; "For Phase 14" section lists the rules (group size is not cause validity, benign kinds justified per kind, late-picture frame by a gap).
- `.planning/phases/13-why-carts-fail/13-CONTEXT.md` - diagnostic design (D-01 to D-07 there), the leak contract and the owner hand-off (D-22).
- `.planning/ROADMAP.md` (Phase 14, Phase 15) and `.planning/REQUIREMENTS.md` (FIX-01 to FIX-04, Out of Scope) - goal, criteria and fences.
- `.planning/seeds/SEED-016-failing-carts.md`, `.planning/seeds/SEED-006-sg1000-mode.md` - the open question and the legacy-mode seed D-07 leaves dormant.
- `.planning/LESSONS.md` - XFER-044, XFER-045, XFER-054.
- `CLAUDE.md` - rules 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 (never open GPL source; counts only; proportional gates).

### Hardware and decisions
- `.planning/preparation/DECISIONS.md` - DEC.03 (SG-1000 refused until SEED-006), DEC.37 (banks wrap modulo pages), DEC.39 and DEC.40 (bounded run; first fault captured), DEC.42, DEC.16.
- `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` section 2 (mapper registers, `$FFFC` bits, odd image sizes) and `SMS-SYSTEM.md` sections 4 and C5 (port decode, open bus).
- `.planning/preparation/SMS-VDP.md` sections 1 and 7 (legacy modes) and `docs/hardware/`.
- `docs/boards.md`, `docs/embedding.md` (fault table and the "never faults" principle, around line 343), `docs/runner.md` (boots text), `docs/conformance.md` (judge and diagnose text), `docs/testing.md`, `docs/maintaining.md`.

### Code
- `src/mapper.c`, `src/mapper_ext.c`, `src/cart.c`, `src/bus.c`, `src/io.c`, `src/fault.c`, `src/vdp_render.c` (`vdp_supported`, the raise in `render_until`), `src/vdp.c` (display latch), `src/state.c`, `include/cogmaster.h`.
- `runner/expect.c` (`cogmaster__expect_boots`), `runner/diag.c`, `scripts/library.sh`, `scripts/library.cmake`.
- `tests/cmake/hashes.cmake`, `tests/testcard_pins.cmake`, `tests/diag/expected.csv`, `tests/library/tests.cmake`.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- The Phase 13 diagnostic (`runner/diag.c`, `library.sh diagnose`) with its closed-vocabulary line, validator and leak canary: extend it, do not build a second tool.
- Hand-assembled guest fixtures in `tests/bus/test_bus.c` (`load_sega`: `LD A,v; LD ($FFFC),A; HALT`) and the `static-display` and never-draws seeds.
- The hashes compare mode in `tests/cmake/hashes.cmake` for base-versus-head pin proof.

### Established Patterns
- The fault record keeps only the first fault, so policy belongs in the core recorder, not the runner's predicate.
- Every printed value is a number, an enum name or a catalog title; nothing from a dump is written to a file.
- Every gate names the fault it prevents; negative controls assert a specific failure.

### Integration Points
- Raise sites: `src/mapper.c` (about lines 43, 111-113, 126, 140), `src/mapper_ext.c` (about lines 34, 102-193), `src/io.c` (about lines 197, 210, 463, 508), `src/bus.c` (186, 255, 260), `src/vdp_render.c` (about 190).
- Revision: `include/cogmaster.h:16`, `tests/core/test_core.c:434`, `src/state.c:109`.
- Footgun: the core applies the Sega shift to every image size; `pcslot 1` means init code in the first 16 KiB slot of the image, not mapper slot 1.

</code_context>

<specifics>
## Specific Ideas

- Judge each group by its own leaf shrinking, and print before and after counts in the outcomes note; a small flip count after fixing the biggest group is not a failed fix.
- Label any new rendering or decode claim "selected model", never "measured" or "chip-exact".

</specifics>

<deferred>
## Deferred Ideas

- A TMS9918 Graphics I and II renderer and SG-1000 mode: SEED-006, a phase of its own; the `vdpmode` counts from D-07 are its trigger evidence.
- Gating the Sega bank shift by image size, unless D-06's trace shows it is needed.
- A public counter or fault flag for benign events: rejected for now (header growth for one consumer); the diagnostic cross-cuts carry the signal.
- A Start-press rerun campaign beyond the one `static_no_fault` row.

</deferred>

---

*Phase: 14-Fix the causes*
*Context gathered: 2026-11-12*
