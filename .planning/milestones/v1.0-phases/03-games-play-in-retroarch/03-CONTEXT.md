# Phase 03: Games play in RetroArch - Context

**Gathered:** 2026-10-20
**Status:** Ready for planning
**Runtime / model:** Codex / GPT-6

<domain>
## Phase Boundary

GAME-01 through GAME-07: mode-4 rendering, interrupt/counter/status timing, native Game Gear and its SMS
compatibility mode, two SMS pads, regions, battery continuation, fixture play in RetroArch, and the compatibility
ledger/local library preset. Sound is phase 4. BIOS boot, extra boards/peripherals and measured save interop
remain later work. Library, runner and libretro core remain the only deliverables.

The owner selected all four discussion areas and explicitly delegated the decisions: fan out across relevant
stakeholders, use primary research, consider alternatives and an adversarial pass, synthesize coherent
recommendations, and follow them without another approval round. Prefer existing code and small permissive
seeds to dependencies. Three read-only research agents covered presentation, input/saves and compatibility;
the parent reconciled their recommendations. This step produces context only; it does not start planning.

</domain>

<decisions>
## Implementation Decisions

### Picture presentation and rendering

- **D-01:** Keep the released default Game Gear PAR 6:5 (160:144 yields DAR 4:3). Offer an explicitly named
  square-pixel presentation option (native DAR 10:9) in the adapter. Neither changes pixels, timing, machine
  selection or hashes. HOSTS §1's square-default recommendation loses to the released behavior documented in
  that same section and ARCHITECTURE §5; there is no new panel measurement justifying a default reversal.
- **D-02:** Native GG crop remains the default; full VDP field is an optional presentation view. Never enter
  SMS compatibility mode merely to expose offscreen pixels. Apply the selected PAR to the displayed extent,
  rather than stretching every extent to 4:3. Label full-field viewing as exposing normally hidden content.
  SMS stays 256×192/224/240, including the hardware mask column; borders and automatic left-column crop wait.
- **D-03:** Keep RGB444 field/crop identity and host palette conversion. Retain the no-content test-card
  oracle unchanged. Report PAL's documented PAR rather than returning NTSC 8:7 for both standards. The field
  buffer is bounded by 256×240; GG 192/224-line crop origins are (48,24)/(48,40). Preserve the documented
  GG 240-line LCD freeze as the best-supported default, confidence-qualified and tested for deterministic,
  bounded delivery. Keep LCD behavior distinct from full-field rendering; do not invent a hardware crop.
- **D-04:** Return consistent dimensions, pitch, crop and aspect for a completed field, even through mode
  changes. Use libretro SET_GEOMETRY inside retro_run when displayed geometry/aspect changes; max storage is
  established at load and never grows. A geometry notification refusal must not stop bounded frame delivery.
  Exact register latch policy is research/planner discretion grounded in SMS-VDP, not an owner question.
- **D-05:** Keep SMS2 default, with SMS1 and GG differences in shared profiles. Expose the in-scope profile
  and GG SMS-mode overrides coherently through media/runner/core options; auto detection uses table/header
  facts. Unsupported TMS/SG modes remain unsupported, not a second renderer added under this phase.
- **D-06:** Keep the settled line renderer and monotonic catch-up before visible mutations. Bound work by
  the field, 64 sprite entries, eight visible sprite slots and emitted segments; a hostile write loop must not
  repeatedly redraw earlier pixels, allocate memory, or postpone the fixed frame deadline. Preserve the
  phase-2 instruction-boundary overshoot guarantee through height/interrupt changes.
- **D-07:** Keep named 607/608 interrupt constants, 600 latch and 604 V-counter defaults unless stronger
  evidence changes the model. The newly accessible final posts of the SMS Display Timing thread reconcile
  the apparent 654 estimate through H-counter origin; research that coordinate before changing an IRQ edge.
  A binary oracle is a locator, never a gate or hardware verdict. Preserve per-frame vscroll and the specific
  SMS1 zoom-slot rule in ARCHITECTURE §13; DEC.44's shorthand "first four" is not a second independent rule.
  Capture assumptions and distinguish tests of our selected model from measurements of real hardware.

### Reset and input behavior

- **D-08:** Separate RetroArch Restart from a physical Reset input. Restart invokes existing SOFT reset on
  SMS and POWER on GG, preserving battery bytes and generation in both cases. Port-1 Select is the physical
  SMS Reset level on profiles that have that switch. It changes the pin, never resets the CPU directly.
  This resolves contradictory HOSTS §1 / ARCHITECTURE §9 wording: libretro.h prefers soft reset, and a Restart
  that only pulses an ignored game input can fail to restart. POWER everywhere is the acceptable alternative
  only if a demonstrated software case needs it. Do not change the library's existing SOFT/POWER contracts.
- **D-09:** Keep D-pad, B=1, A=2 and port-1 Start=SMS Pause/native-GG Start. Console controls have one source
  (port 1); SMS has two independent pads, native GG one local pad. GG SMS compatibility mode maps Start to
  Pause/NMI per its hardware model. Advertise only pad/none devices that actually work in this phase.
- **D-10:** Pass independent direction bits through, including U+D and L+R. SOCD cleaning is a host policy;
  last-input priority would add hidden ordering and state. Poll fresh absolute input once before each frame.
  Missing callbacks, disconnected devices and RETRO_DEVICE_NONE release pins immediately and cannot leave
  stuck buttons. A reconnect does not manufacture a Pause edge from stale host state.
- **D-11:** Core edge-detects Pause; holding it produces one NMI at the settled last-line timing. Native GG
  Start is an active-low port level, not an NMI. Lag credit comes only from actual CPU reads of controller
  ports that the active profile decodes, including valid mirrors; peeks and host polling do not count. Resolve
  GG alias claims against the full-decode model during research instead of crediting undriven reads.
- **D-12:** Runner --input remains the headerless body of the future movie format, with one absolute record
  per frame. Choose this small textual grammar: `p1=00 p2=00 pause=0 reset=0 start=0`. Pad masks are two hex
  digits, bits 0..5 = U,D,L,R,1,2; high bits are invalid. Console fields are 0/1. Accept LF/CRLF and an optional
  final newline; no blank/comment records in the body. Without --input all inputs are released. Require exactly
  --frames records; reject unknown fields/bits, overlong lines, early EOF and surplus records as input errors.
  Use a bounded parser and existing file handling, not JSON, BK2 or a replay dependency. --port1/--port2 accept
  the in-scope pad/none choices; none suppresses that pad. Frame numbering and boundary application must be
  documented with a tiny example and tested at the first press/release. Future movie headers remain deferred.

### Battery identification, import and persistence

- **D-13:** At load, classify physical mapper RAM separately from persistent media using the public pinned
  CC0 hash metadata. Known volatile Sega/Codemasters RAM stays out of BATTERY/SAVE_RAM. Export exactly the
  classified persistent length, fixed pointer and length until unload; never change exposure on first RAM
  enable. Unknown ROMs can still run through supported heuristics, but persistence is explicitly unknown and
  no battery span is guessed. An explicit --save-dir request for unknown persistence is refused before running
  (exit 77 with the reason), avoiding silent loss. Known nonbattery titles need no file. General homebrew
  persistence overrides are deferred until an actual use case needs them.
- **D-14:** The own battery fixture has explicit hash-keyed metadata consumed by the production classification
  path, separate in provenance from CC0 commercial data. Store its reproducible expected identity as a fixture
  fact and verify it against the assembler result; do not introduce a core-build/fixture-build dependency cycle,
  a filename match or a test-only loader bypass. Public CC0 titles absent from the ledger remain identifiable.
- **D-15:** Verify the claimed 24 KiB entry against the pinned list, then model physical bank decoding and
  persistent length independently. Existing `ram_size - 1` masking assumes powers of two: assigning 24576 to
  that field is invalid. Exact exports do not authorize truncating physical RAM or inventing modulo decoding.
  Research settles the mapping from documented board facts; cover supported sizes/banks with independent tests.
- **D-16:** Raw import: missing file means fresh battery; shorter files (including empty) pad with $FF;
  oversized files reject without applying bytes (runner exit 4). Read at most the expected length plus one.
  Other I/O errors fail with exit 1. Exact-size wrong-game data cannot be detected by a raw format: preserve
  that format and use a wrong-save control that produces a different continuation, not a fake validation claim.
- **D-17:** Runner saves to an existing --save-dir as `<ROM stem>.srm`, consistently with CARTRIDGES §4 and
  RetroArch. Resolve HOSTS §4's deferred Playstead `.sav` example in favor of .srm for this runner. No automatic
  dual-extension import or hash namespace. Explain same-stem collision behavior and use separate save dirs.
  Reject --save-dir without content. The libretro adapter exports memory; the frontend owns file I/O.
- **D-18:** Increment the 64-bit generation on each mapped persistent write, including same-value writes.
  Reads, RAM-enable writes, volatile writes and import do not increment it. Snapshot after import; flush only
  dirty generations and acknowledge persistence only after successful replacement. Reset preserves bytes and
  generation. API additions need actual consumers/tests under the existing size-tag and version rules.
- **D-19:** Persist with a uniquely and exclusively created temporary in the destination directory, checked
  full write/flush/close and platform replacement semantics. Never delete/truncate the last good target first;
  clean up only this process's temp. Use POSIX/native Windows facilities, not a filesystem package. Finish the
  bounded frame after SIGTERM/SIGINT, then flush outside the signal handler; exit 0 only on success, exit 1 on
  failure. Test first create, overwrite and failure preservation. Request synchronization where supported, but
  distinguish atomic visibility from universal power-loss durability. Concurrent writers to one save are not a
  supported multiwriter protocol; no locks, journal, backup rotation or daemon is added in this phase.

### Compatibility reporting, fixtures and verification

- **D-20:** A result asserts its predicate at a frame cap/profile/revision. `boots@F` is a strict boot smoke
  observation, never a playable/completable claim. Keep its display/distinct-frame/first-fault observations
  explicit; a static screen may fail this heuristic without being a broken game. Remove the instantaneous
  "CPU not HALTed" rejection: phase 2 proves healthy interrupt-driven HALT loops. Do not replace it with an
  invented general stall detector. Fault records stay diagnostic; a failed strict smoke check is not proof
  of a gameplay defect. No sound claim before phase 4.
- **D-21:** Keep required `tests/compat/ledger.csv` and its nine logical columns. Use conventional quoted CSV
  for titles containing commas/quotes, with doubled embedded quotes. The pinned CC0 titles demonstrate that
  CONFORMANCE §5's no-comma rule conflicts with verbatim titles. Share a small parser under the CMake/POSIX-sh
  repository-check rules; handle semicolons without treating them as CMake list separators. No CSV dependency.
- **D-22:** Ratchet identity includes ROM, resolved machine/profile, region/TV, board/device configuration,
  input-script identity and normalized predicate/frame/address parameters. Expected hashes are protected
  values, not part of identity: changing `frame@F=H` must not create a new unprotected case. Protect script
  content as well as its path. Deleted passing cases, duplicate normalized keys, changed expectations and
  profile substitutions cannot bypass comparison. Preserve the dated behavior-revision/expiry exception
  mechanism and protected base-branch judge. Keep these in the existing ratchet, not a new governance service.
- **D-23:** The local library run enumerates every candidate regular image in COGMASTER_ROM_DIR and attempts
  supported images with bounded stepping, even when absent from the CC0 list. Classification/unsupported
  refusals and malformed/I/O cases stay distinct. Without an accepted expectation report unknown plus local
  observations; never make current output its own golden hash. Zero evaluated images must say zero coverage.
  This resolves GAME-06's "every image" versus CONFORMANCE §5's matches-only registration: public matching
  controls export eligibility, not local execution. All enumeration and private diagnostics stay local.
- **D-24:** Only public CC0 identities/titles and allowed predicate facts/date/core revision may enter the
  public ledger through the explicit update path. Unknown names/digests, paths, screenshots, RAM/save bytes
  and commercial traces never enter commits, CI artifacts or evidence. Keep local results outside tracked
  files; never upload the whole library build directory. CI tests this preset using a temporary fixture dir.
- **D-25:** pass/fail describe evaluated predicates; unknown lacks an accepted evaluation/expectation;
  unsupported names the unavailable board/device/profile; infrastructure_error names a failed runner/judge.
  Do not call all refusals game failures or treat table membership as a support guarantee. Generated public
  docs show predicate/fixture counts and their limits. Keep .info experimental through v2.0.
- **D-26:** Extend the named WLA-DX fixtures and mailbox, with on-screen own glyphs inside the GG window.
  boot_to_play proves independent no-input/press/held/repeated-edge cases for both SMS pads and GG Start;
  input changes counter, scroll and frame. vdp_lines covers specified rendering features, heights, counters,
  flags and scroll bands. battery proves fresh-process continuation after exit/SIGTERM, missing/wrong controls.
  Expectations derive from hardware documents and independent calculations, not just two paths to the same bug.
- **D-27:** Reuse/simplify the existing RetroArch screenshot driver before adding content/input/GG coverage:
  tests/retroarch is already 293 of 300 allowed lines. Keep richer semantics in the in-process host and fixtures;
  the hosted session must still genuinely deliver input to the loaded game and compare its scrolled frame.
  Test-only input injection must not become the installed adapter's input source. Keep existing test-card
  coverage while sharing the driver. No stateful shell harness or expanded e2e framework.
- **D-28:** Existing inventory/negative-control, zero-allocation, endian/hash and bounded-run checks carry
  forward. New behavior is tested in ci; one-time facts get command/exit/run URL, never committed command logs.
  Repository-check code stays below one tenth of src lines; consolidate checks rather than waive that budget.
  The representative performance workload becomes boot_to_play; arming the perf gate remains the separate
  post-merge CI follow-up. Review precedes verification; one GSD step ends here.

### Agent's Discretion

- Research and plan the smallest module/API split, mapper metadata generation, native replacement helpers,
  exact option keys consistent with established ones, and fixture decomposition. No new runtime dependency
  is justified by the discussion. Reuse runner hashes, fixture includes, frame conversion and signal handling.
- Resolve measured hardware details (GG aliases, 24 KiB board, H-counter origin, height/latch edge cases)
  through cited research/tests. A source conflict is not a request for owner verification or a license to
  keep changing constants until another emulator agrees. Record confidence and retain best-supported defaults.
- Reconcile stale preparation prose listed above during this phase's PR. These decisions clarify that prose;
  they do not reopen delivery policy, CPU vectors, machine architecture or later milestone boundaries.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read the relevant sections before planning or implementing.**

### Scope, rules and prior choices
- `AGENTS.md`, `.planning/PROJECT.md`, `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md` — phase scope and gates.
- `.planning/LESSONS.md` — especially XFER-007/013/016/017/018/025/044/045/049/055/058/059/061/064/067/073.
- `.planning/preparation/README.md`, `.planning/preparation/DECISIONS.md` — source index and settled decisions.
- `.planning/preparation/BRIEF.md`, `.planning/preparation/GSD-HANDOFF.md` — fan-out method and single-step workflow.
- `.planning/phases/01-a-test-frame-in-retroarch/01-CONTEXT.md` — header growth, runner surface, test-card oracle.
- `.planning/phases/02-the-cpu-matches-the-public-vectors/02-CONTEXT.md` — deadlines, profiles, ports, fixtures.
- `.planning/phases/02-the-cpu-matches-the-public-vectors/02-REVIEW-DISPOSITION.md` — seeds, not extra requirements.
- `PROVENANCE.md`, `ASSET_POLICY.md` — permitted source/data, fixture facts and private-library boundaries.

### Hardware, hosts and conformance
- `.planning/preparation/SMS-VDP.md` §§1–6,9–10 — mode 4, revisions, timing, crop and uncertainty.
- `.planning/preparation/SMS-SYSTEM.md` §§2,4–8 — timing, decode, reset and Pause.
- `.planning/preparation/SMS-CARTRIDGES-AND-PERIPHERALS.md` §§1–4,6 — metadata, pads and battery.
- `.planning/preparation/SMSPOWER-EXTRACTS.md` §§6–7 and cited SP records — fixtures, measurements, assembler.
- `.planning/preparation/ARCHITECTURE.md` §§2–9,11–13 — bounded API, renderer, profiles, memory and conflicts.
- `.planning/preparation/CONFORMANCE.md` §§2–7 — fixtures, mailbox, locators, ledger and library privacy.
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` §§1–5 — adapter, presentation, input and runner.
- `.planning/preparation/ENGINEERING.md` §§3–4,8,10–11 — tests, CI, performance and budgets.
- `libretro/libretro.h` — pinned host contract, especially SET_GEOMETRY, memory lifetime and retro_reset.
- `docs/embedding.md`, `docs/runner.md`, `docs/testing.md` — current public behavior to preserve/extend.

### External primary sources checked during discussion (2026-10-20)
- [Original hardware timing measurements and later H-counter reconciliation](https://www.smspower.org/forums/8161-SMSDisplayTiming).
- [Libretro core contract](https://docs.libretro.com/development/cores/developing-cores/).
- [Gearsystem published behavior](https://docs.libretro.com/library/gearsystem/) and
  [Genesis Plus GX presentation documentation](https://docs.libretro.com/library/genesis_plus_gx/) — docs only.
- [Public pinned MAME SMS CC0 data](https://github.com/mamedev/mame/blob/1d7a454ed79e/hash/sms.xml) — commas in titles;
  verify per-file license and public commit before generating all three settled tables.
- [POSIX rename](https://pubs.opengroup.org/onlinepubs/9799919799/functions/rename.html),
  [POSIX crash-persistence limits](https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap04.html),
  [Windows ReplaceFile](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew) — replacement
  semantics, errors and durability limits; choose the smallest suitable native helper.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/vdp.c`, `src/bus.c`, `src/frame.c` — timing/ports/deadline and crop seams; loaded frames currently lack rendering.
- `src/io.c`, `src/instance.c`, `src/boot.c` — decoded pins/profiles, frame entry and soft/power reset.
- `src/cart.c`, `src/mapper.c`, `src/internal.h` — fixed cart allocation and power-of-two masks to audit for exact media.
- `src/state.c` visitor — extend/validate internal state with new latches/input/generation, without public save states.
- `runner/sha1.c`, `runner/args.c`, `runner/stop.c`, `runner/mailbox.c` — identity, CLI, signal flag, numeric measurements.
- `libretro/libretro.c`, `host/palette.c` — in-process host boundary, RGB conversion and frame delivery.
- `tests/roms/src/mailbox.inc`, `tests/roms/src/header.inc`, `tests/diag/expected.csv` — own fixture scaffolding.
- `tests/libretro/host.c`, `tests/retroarch/run_retroarch.cmake`, `tests/cmake/expect_fail.cmake` — existing judges.

### Established Patterns and Integration Points
- Size-tagged API, append-only statuses and one API-version increment with actual additions; no speculative getters.
- Per-subsystem tests.cmake, exact inventories, tolerant regexes and meaningful negative controls.
- No float/core globals/allocation after load; shared profiles and switch dispatch, caller/host memory ownership.
- Runner --expect adds exit 3 for predicate mismatch; invalid battery length adds exit 4 when implemented. Keep the
  one exit table/docs/help coherent; first-fault tracing remains data with its existing status semantics.
- Extend fixture/hash equality and hosted cases in existing CI; never register private library tests in ci.

</code_context>

<specifics>
## Specific Ideas

- Owner's priority: broad relevant lenses, concrete alternatives, adversarial synthesis and one recommendation
  per decision. Avoid irrelevant technology analogies, repeated confirmations and dependency abstractions.
- New evidence justifies CSV/parser and ratchet-identity corrections; the existing healthy HALT fixture justifies
  removing the sampled-HALT rejection. These are bounded corrections, not a new compatibility program.

</specifics>

<deferred>
## Deferred Ideas

- Phase 4 audio and OPLL; v2 BIOS, peripherals, extra boards and binary-measured save interop; v3 public states
  and versioned movies; v4 measured per-dot accuracy; v5 hooks/debuggers/importers. No new phase is proposed.
- Generic unknown-homebrew persistence overrides, save namespaces/sidecars and multiwriter coordination require
  a demonstrated need. Existing unknown images still run without a false persistence guarantee.

### Reviewed Todos (not folded)
- `.planning/todos/pending/post-merge-ci-follow-ups.md` remains its separate ci: PR: perf arming, nightly reporter,
  Dependabot and wasm promotion are already decided there. No release or discuss dependency is invented here.

</deferred>

---

*Phase: 03-games-play-in-retroarch*
*Context gathered: 2026-10-20*
